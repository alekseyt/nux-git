// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2011 Inalogic® Inc.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License, as
 * published by the  Free Software Foundation; either version 2.1 or 3.0
 * of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License along with this program. If not, see <http://www.gnu.org/licenses/>
 *
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 *
 */

#include "Logger.h"
#include "LoggingWriter.h"

#include <map>
#include <sstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/utility.hpp>

namespace nux {
namespace logging {

namespace {
char const* str_level(Level severity);
}

class LoggerModule
{
public:
  LoggerModule(std::string const& module, LoggerModulePtr const& parent);

  std::string const& module() const;

  bool IsErrorEnabled() const;
  bool IsWarningEnabled() const;
  bool IsInfoEnabled() const;
  bool IsDebugEnabled() const;
  bool IsTraceEnabled() const;

  void SetLogLevel(Level level);
  Level GetLogLevel() const;
  Level GetEffectiveLogLevel() const;

private:
  std::string module_;
  Level level_;
  LoggerModulePtr parent_;
};

class LoggerModules : boost::noncopyable
{
public:
  static LoggerModules& Instance();

  LoggerModulePtr const& GetModule(std::string const& module);

  void reset();
  std::string dump_logging_levels(std::string const& prefix);

private:
  LoggerModules();

private:
  typedef std::map<std::string, LoggerModulePtr> ModuleMap;
  ModuleMap modules_;
  LoggerModulePtr root_;
};


inline std::string const& LoggerModule::module() const
{
  return module_;
}

inline bool LoggerModule::IsErrorEnabled() const
{
  return GetEffectiveLogLevel() <= ERROR;
}

inline bool LoggerModule::IsWarningEnabled() const
{
  return GetEffectiveLogLevel() <= WARNING;
}

inline bool LoggerModule::IsInfoEnabled() const
{
  return GetEffectiveLogLevel() <= INFO;
}

inline bool LoggerModule::IsDebugEnabled() const
{
  return GetEffectiveLogLevel() <= DEBUG;
}

inline bool LoggerModule::IsTraceEnabled() const
{
  return GetEffectiveLogLevel() <= TRACE;
}

inline void LoggerModule::SetLogLevel(Level level)
{
  // The root module can't be unspecified.
  if (module_ == "" && level == NOT_SPECIFIED)
    level = WARNING;
  level_ = level;
}

inline Level LoggerModule::GetLogLevel() const
{
  return level_;
}

inline Level LoggerModule::GetEffectiveLogLevel() const
{
  if (level_ == NOT_SPECIFIED && parent_)
    return parent_->GetEffectiveLogLevel();
  else
    return level_;
}


Logger::Logger(std::string const& module)
  : pimpl(LoggerModules::Instance().GetModule(module))
{
}

std::string const& Logger::module() const
{
  return pimpl->module();
}

bool Logger::IsErrorEnabled() const
{
  return pimpl->IsErrorEnabled();
}

bool Logger::IsWarningEnabled() const
{
  return pimpl->IsWarningEnabled();
}

bool Logger::IsInfoEnabled() const
{
  return pimpl->IsInfoEnabled();
}

bool Logger::IsDebugEnabled() const
{
  return pimpl->IsDebugEnabled();
}

bool Logger::IsTraceEnabled() const
{
  return pimpl->IsTraceEnabled();
}

void Logger::SetLogLevel(Level level)
{
  pimpl->SetLogLevel(level);
}

Level Logger::GetLogLevel() const
{
  return pimpl->GetLogLevel();
}

Level Logger::GetEffectiveLogLevel() const
{
  return pimpl->GetEffectiveLogLevel();
}


LoggerModule::LoggerModule(std::string const& module,
                           LoggerModulePtr const& parent)
  : module_(module)
  , level_(NOT_SPECIFIED)
  , parent_(parent)
{
}

LoggerModules::LoggerModules()
  : root_(new LoggerModule("", LoggerModulePtr()))
{
  // Make sure we have the root logger available.
  root_->SetLogLevel(WARNING);
  modules_.insert(ModuleMap::value_type("", root_));
}

LoggerModules& LoggerModules::Instance()
{
  static LoggerModules instance;
  return instance;
}

LoggerModulePtr const& LoggerModules::GetModule(std::string const& module)
{
  std::string lower_module = boost::to_lower_copy(module);
  ModuleMap::iterator i = modules_.find(lower_module);
  if (i != modules_.end())
    return i->second;

  // Make the new LoggerModule and its parents.
  // Split on '.'
  std::string::size_type idx = lower_module.rfind(".");
  LoggerModulePtr parent = root_;
  if (idx != std::string::npos) {
    parent = GetModule(lower_module.substr(0, idx));
  }
  LoggerModulePtr logger(new LoggerModule(lower_module, parent));
  // std::map insert method returns a pair<iterator, bool> which seems
  // overly annoying to make a temporary of, so just return the const
  // reference pointed to by the interator.
  return modules_.insert(ModuleMap::value_type(lower_module, logger)).first->second;
}

void LoggerModules::reset()
{
  for (ModuleMap::iterator i = modules_.begin(), end = modules_.end(); i != end; ++i)
  {
    i->second->SetLogLevel(NOT_SPECIFIED);
  }
}

std::string LoggerModules::dump_logging_levels(std::string const& prefix)
{
  std::ostringstream sout;
  bool first = true;
  for (ModuleMap::iterator i = modules_.begin(), end = modules_.end(); i != end; ++i)
  {
    std::string const& module_name = i->first;
    LoggerModulePtr const& module = i->second;
    Level severity = module->GetLogLevel();
    if (severity == NOT_SPECIFIED)
      continue; // Don't write out unspecified ones.
    if (first)
      first = false;
    else
      sout << "\n";
    sout << prefix;
    if (module_name == "")
      sout << "<root>";
    else
      sout << module_name;
    sout << " " << str_level(severity);
  }
  return sout.str();
}


class LogStreamBuffer : public std::stringbuf
{
public:
  LogStreamBuffer(Level severity,
                  std::string const& module,
                  std::string const& filename,
                  int line_number);
protected:
  virtual int sync();
private:
  Level severity_;
  std::string module_;
  std::string filename_;
  int line_number_;
  std::time_t timestamp_;
};

LogStream::LogStream(Level severity,
                     std::string const& module,
                     std::string const& filename,
                     int line_number)
  : std::ostream(new LogStreamBuffer(severity, module,
                                     filename, line_number))
{
}

LogStream::~LogStream()
{
  rdbuf()->pubsync();
  std::streambuf* buff = rdbuf(0);
  delete buff;
}


LogStreamBuffer::LogStreamBuffer(Level severity,
                                 std::string const& module,
                                 std::string const& filename,
                                 int line_number)
  : std::stringbuf(std::ios_base::out)
  , severity_(severity)
  , module_(module)
  , filename_(filename)
  , line_number_(line_number)
  , timestamp_(std::time(0))
{
}

int LogStreamBuffer::sync()
{
  std::string message = str();
  // reset the stream
  str("");
  // Only log the message if there is something there.
  if (!message.empty())
    Writer::Instance().WriteMessage(severity_, module_,
                                    filename_, line_number_,
                                    timestamp_, message);
  return 0; // success
}

/**
 * A helper function for testing.  Not exported, but used in tests.
 *
 * Resets the root logger to warning, and sets all other modules to not
 * specified.
 */
void reset_logging()
{
  LoggerModules::Instance().reset();
}

std::string dump_logging_levels(std::string const& prefix)
{
  return LoggerModules::Instance().dump_logging_levels(prefix);
}

void configure_logging(const char* config_string)
{
  if (!config_string)
    return;
  std::vector<std::string> values;
  boost::split(values, config_string, boost::is_any_of(";:"));
  for (std::vector<std::string>::iterator i = values.begin(), end = values.end();
       i != end; ++i)
  {
    std::string& value = *i;
    std::string::size_type pos = value.find("=");
    if (pos != std::string::npos)
    {
      std::string name = value.substr(0, pos);
      std::string level = value.substr(pos+1);
      if (name == "<root>")
        name = "";
      Logger(name).SetLogLevel(get_logging_level(level));
    }
  }
}

Level get_logging_level(std::string level)
{
  boost::to_upper(level);
  if (level == "TRACE")
    return TRACE;
  if (level == "DEBUG")
    return DEBUG;
  if (level == "INFO")
    return INFO;
  if (level == "WARN" || level == "WARNING")
    return WARNING;
  if (level == "ERROR")
    return ERROR;
  return WARNING;
}

BlockTracer::BlockTracer(Logger& logger,
                         Level level,
                         std::string const& function_name,
                         std::string const& filename,
                         int line_number)
  : logger_(logger)
  , level_(level)
  , function_name_(function_name)
  , filename_(filename)
  , line_number_(line_number)
{
  if (logger_.GetEffectiveLogLevel() >= level_)
  {
    LogStream(level_, logger_.module(), filename_, line_number_).stream()
      << "+" << function_name_;
  }
}

BlockTracer::~BlockTracer()
{
  if (logger_.GetEffectiveLogLevel() >= level_)
  {
    LogStream(level_, logger_.module(), filename_, line_number_).stream()
      << "-" << function_name_;
  }
}


namespace {
char const* str_level(Level severity)
{
  switch (severity)
  {
  case NOT_SPECIFIED:
    return "NOT_SPECIFIED";
  case TRACE:
    return "TRACE";
  case DEBUG:
    return "DEBUG";
  case INFO:
    return "INFO";
  case WARNING:
    return "WARNING";
  case ERROR:
    return "ERROR";
  case CRITICAL:
    return "CRITICAL";
  }
  return "<unknown>";
}

}

} // namespace logging
} // namespace nux
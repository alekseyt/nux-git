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
#ifndef NUX_CORE_LOGGER_H
#define NUX_CORE_LOGGER_H

#include <ostream>
#include <string>
#include <boost/shared_ptr.hpp>

#define LOG_DEBUG(logger) \
  if (!logger.IsDebugEnabled()) {} \
  else ::nux::logging::LogStream(::nux::logging::DEBUG, logger.module(), __FILE__, __LINE__).stream()
#define LOG_INFO(logger) \
  if (!logger.IsInfoEnabled()) {} \
  else ::nux::logging::LogStream(::nux::logging::INFO, logger.module(), __FILE__, __LINE__).stream()
#define LOG_WARN(logger) LOG_WARNING(logger)
#define LOG_WARNING(logger) \
  if (!logger.IsWarningEnabled()) {} \
  else ::nux::logging::LogStream(::nux::logging::WARNING, logger.module(), __FILE__, __LINE__).stream()
#define LOG_ERROR(logger) \
  if (!logger.IsErrorEnabled()) {} \
  else ::nux::logging::LogStream(::nux::logging::ERROR, logger.module(), __FILE__, __LINE__).stream()

namespace nux {
namespace logging {

enum Level
{
  NOT_SPECIFIED,
  TRACE,
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  CRITICAL,
};

// Convert a string representation of a logging level into the enum value.
Level get_logging_level(std::string level);

class LogStream : public std::ostream
{
public:
  LogStream(Level severity,
            std::string const& module,
            std::string const& filename,
            int line_number);
  ~LogStream();

  std::ostream& stream() { return *this; }
};


class LoggerModule;
typedef boost::shared_ptr<LoggerModule> LoggerModulePtr;

class Logger
{
public:
  explicit Logger(std::string const& module);

  std::string const& module() const;

  bool IsErrorEnabled() const;
  bool IsWarningEnabled() const;
  bool IsInfoEnabled() const;
  bool IsDebugEnabled() const;

  void SetLogLevel(Level level);
  Level GetLogLevel() const;
  Level GetEffectiveLogLevel() const;

private:
  LoggerModulePtr pimpl;
};

}
}


#endif

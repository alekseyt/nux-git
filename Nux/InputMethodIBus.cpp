/*
* Copyright 2010 Inalogic® Inc.
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
* Authored by: Brandon Schaefer <brandontschaefer@gmail.com>
*              Jay Taoko <jaytaoko@inalogic.com>
*
*/


#include "Nux.h"

#include "InputMethodIBus.h"

namespace nux
{

  std::vector<Event> IBusIMEContext::hotkeys_;

  IBusBus* IBusIMEContext::bus_ = NULL;

  IBusIMEContext::IBusIMEContext(TextEntry* text_entry)
    : text_entry_(text_entry),
      context_(NULL),
      is_focused_(false)
  {
    // init ibus
    if (!bus_)
    {
      ibus_init();
      bus_ = ibus_bus_new();
    }

    // connect bus signals
    g_signal_connect(bus_, "connected",    G_CALLBACK(OnConnected_),     this);
    g_signal_connect(bus_, "disconnected", G_CALLBACK(OnDisconnected_),  this);

    if (ibus_bus_is_connected(bus_))
    {
      CreateContext();
    }
    else
    {
      nuxDebugMsg("[IBusIMEContext::IBusIMEContext] Can not connect to ibus");
    }
  }

  IBusIMEContext::~IBusIMEContext()
  {
    // disconnect bus signals
    g_signal_handlers_disconnect_by_func(bus_, reinterpret_cast<gpointer>(OnConnected_),    this);
    g_signal_handlers_disconnect_by_func(bus_, reinterpret_cast<gpointer>(OnDisconnected_), this);

    DestroyContext();
  }

  void IBusIMEContext::Focus()
  {
    if (is_focused_)
      return;
    is_focused_ = true;

    if (context_)
      ibus_input_context_focus_in(context_);
  }

  void IBusIMEContext::Blur()
  {
    if (!is_focused_)
      return;

    is_focused_ = false;

    if (context_)
      ibus_input_context_focus_out(context_);
  }

  void IBusIMEContext::Reset()
  {
    if (context_)
      ibus_input_context_reset(context_);
  }

  bool IBusIMEContext::FilterKeyEvent(const KeyEvent& event)
  {
    guint keyval = event.key_sym(); // todo(jaytaoko): ui::GdkKeyCodeForWindowsKeyCode(event.key_code(), event.IsShiftDown() ^ event.IsCapsLockDown());

    if (context_) {
      guint modifiers = 0;

      if (event.flags() & IBUS_IGNORED_MASK)
        return false;

      if (event.type() == EVENT_KEY_UP)
        modifiers |= IBUS_RELEASE_MASK;

      if (event.IsShiftDown())
        modifiers |= IBUS_SHIFT_MASK;
      if (event.IsControlDown())
        modifiers |= IBUS_CONTROL_MASK;
      if (event.IsAltDown())
        modifiers |= IBUS_MOD1_MASK;
      if (event.IsCapsLockDown())
        modifiers |= IBUS_LOCK_MASK;

      ibus_input_context_process_key_event_async(context_,
        keyval, event.key_code() - 8, modifiers,
        -1,
        NULL,
        reinterpret_cast<GAsyncReadyCallback>(ProcessKeyEventDone),
        new ProcessKeyEventData(this, event));
      return true;
    }
    return false;
  }

  void IBusIMEContext::SetSurrounding(const std::wstring& text, int cursor_pos)
  {
      // TODO(penghuang) support surrounding
  }

  void IBusIMEContext::CreateContext() {
    nuxAssert(bus_ != NULL);
    nuxAssert(ibus_bus_is_connected(bus_));

    if (!(context_ = ibus_bus_create_input_context(bus_, "nux")))
    {
      nuxDebugMsg("[IBusIMEContext::IBusIMEContext] Cannot create InputContext");
      return;
    }

    text_entry_->ime_active_ = false;

    // connect input context signals
    g_signal_connect(context_, "commit-text",         G_CALLBACK(OnCommitText_),        this);
    g_signal_connect(context_, "update-preedit-text", G_CALLBACK(OnUpdatePreeditText_), this);
    g_signal_connect(context_, "show-preedit-text",   G_CALLBACK(OnShowPreeditText_),   this);
    g_signal_connect(context_, "hide-preedit-text",   G_CALLBACK(OnHidePreeditText_),   this);
    g_signal_connect(context_, "enabled",             G_CALLBACK(OnEnable_),            this);
    g_signal_connect(context_, "disabled",            G_CALLBACK(OnDisable_),           this);
    g_signal_connect(context_, "destroy",             G_CALLBACK(OnDestroy_),           this);

    guint32 caps = IBUS_CAP_PREEDIT_TEXT |
      IBUS_CAP_FOCUS |
      IBUS_CAP_SURROUNDING_TEXT;
    ibus_input_context_set_capabilities(context_, caps);

    if (is_focused_)
      ibus_input_context_focus_in(context_);

    UpdateCursorLocation();

    IBusConfig* bus_conf = ibus_bus_get_config(bus_);
    g_signal_handlers_disconnect_by_func(bus_conf, reinterpret_cast<gpointer>(OnConfigChanged_), this);
    g_signal_connect(bus_conf, "value-changed", G_CALLBACK(OnConfigChanged_), this);
    UpdateHotkeys();
  }

  void IBusIMEContext::DestroyContext()
  {
    //nuxDebugMsg("***IBusIMEContext::DestroyContext***");

    if (ibus_bus_is_connected(bus_))
    {
      IBusConfig* bus_conf = ibus_bus_get_config(bus_);
      g_signal_handlers_disconnect_by_func(bus_conf, reinterpret_cast<gpointer>(OnConfigChanged_), this);
    }

    if (!context_)
      return;

    text_entry_->ResetPreedit();
    ibus_proxy_destroy(reinterpret_cast<IBusProxy *>(context_));

    nuxAssert(!context_);
  }

  bool IBusIMEContext::IsConnected() const
  {
    return context_;
  }

  void IBusIMEContext::UpdateCursorLocation()
  {
    nux::Rect strong, weak;
    text_entry_->GetCursorRects(&strong, &weak);

    // Get the position of the TextEntry in the Window.
    nux::Geometry geo = text_entry_->GetAbsoluteGeometry();

    // Get the Geometry of the window on the display.
    nux::Geometry window_geo = nux::GetGraphicsDisplay()->GetWindowGeometry();

    ibus_input_context_set_cursor_location(context_,
      geo.x + window_geo.x + strong.x,
      geo.y + window_geo.y,
      0,
      geo.height);
  }

  void IBusIMEContext::OnConnected(IBusBus *bus)
  {
    //nuxDebugMsg("***IBusIMEContext::OnConnected***");
    nuxAssert(bus_ == bus);
    if (ibus_bus_is_connected(bus_))
    {
      DestroyContext();
      CreateContext();
    }
  }

  void IBusIMEContext::OnDisconnected(IBusBus *bus)
  {
    //nuxDebugMsg("***IBusIMEContext::OnDisonnected***");
    hotkeys_.clear();

    DestroyContext();
  }

  void IBusIMEContext::OnConfigChanged(IBusConfig* config, gchar* section, gchar* name, GVariant* value)
  {
    if (g_strcmp0(section, "general/hotkey") == 0)
    {
      if (g_strcmp0(name, "trigger") == 0)
        UpdateHotkeys();
    }
  }

  void IBusIMEContext::OnCommitText(IBusInputContext *context, IBusText* text)
  {
    //nuxDebugMsg("***IBusIMEContext::OnCommitText::%s***", text->text);
    nuxAssert(context_ == context);

    text_entry_->DeleteSelection();

    if (text->text)
    {
      int cursor = text_entry_->cursor_;
      std::string new_text(text_entry_->GetText());
      std::string commit_text (text->text);
      new_text.insert (cursor, commit_text);

      text_entry_->SetText(new_text.c_str());
      text_entry_->SetCursor(cursor + commit_text.length());
      UpdateCursorLocation();
    }
  }

  void IBusIMEContext::OnUpdatePreeditText(IBusInputContext* context, IBusText* text, guint cursor_pos, gboolean visible)
  {
    //nuxDebugMsg("***IBusIMEContext::OnUpdatePreeditText***");
    nuxAssert(context_ == context);
    nuxAssert(IBUS_IS_TEXT(text));

    if (text_entry_->preedit_.empty())
      UpdateCursorLocation();

    if (visible)
    {
      IBusAttrList* attrs = text->attrs;
      if (attrs)
      {
        PangoAttrList* preedit_attrs = pango_attr_list_new();
        int i = 0;
        while (1)
        {
          IBusAttribute* attr = ibus_attr_list_get(attrs, i++);
          PangoAttribute* pango_attr;
          if (!attr)
            break;
          switch (attr->type)
          {
             case IBUS_ATTR_TYPE_UNDERLINE:
              pango_attr = pango_attr_underline_new ((PangoUnderline)attr->value);
              break;
            case IBUS_ATTR_TYPE_FOREGROUND:
              pango_attr = pango_attr_foreground_new (
                            ((attr->value & 0xff0000) >> 8) | 0xff,
                            ((attr->value & 0x00ff00)) | 0xff,
                            ((attr->value & 0x0000ff) << 8) | 0xff);
              break;
            case IBUS_ATTR_TYPE_BACKGROUND:
              pango_attr = pango_attr_background_new (
                            ((attr->value & 0xff0000) >> 8) | 0xff,
                            ((attr->value & 0x00ff00)) | 0xff,
                            ((attr->value & 0x0000ff) << 8) | 0xff);
              break;
            default:
              continue;
          }
          pango_attr->start_index = g_utf8_offset_to_pointer (text->text, attr->start_index) - text->text;
          pango_attr->end_index = g_utf8_offset_to_pointer (text->text, attr->end_index) - text->text;
          pango_attr_list_insert (preedit_attrs, pango_attr);
        }
        if (text_entry_->preedit_attrs_)
        {
          pango_attr_list_unref(text_entry_->preedit_attrs_);
          text_entry_->preedit_attrs_ = NULL;
        }
        text_entry_->preedit_attrs_ = preedit_attrs;
      }
      if (text->text)
      {
        std::string preedit(text->text);
        text_entry_->preedit_ = preedit;
        text_entry_->preedit_cursor_ = preedit.length();
        text_entry_->QueueRefresh (true, true);
      }
    }
    else
    {
      OnHidePreeditText(context_);
    }
  }

  void IBusIMEContext::OnShowPreeditText(IBusInputContext *context)
  {
    //nuxDebugMsg("***IBusIMEContext::OnShowPreeditText***");
    nuxAssert(context_ == context);
  }

  void IBusIMEContext::OnHidePreeditText(IBusInputContext *context)
  {
    //nuxDebugMsg("***IBusIMEContext::OnHidePreeditText***");
    nuxAssert(context_ == context);

    text_entry_->ResetPreedit();
    text_entry_->QueueRefresh (true, true);
  }

  void IBusIMEContext::OnEnable(IBusInputContext *context)
  {
    //nuxDebugMsg("***IBusIMEContext::OnEnable***");
    nuxAssert(context_ == context);

    text_entry_->ime_active_ = true;
    text_entry_->text_changed.emit(text_entry_);
    UpdateCursorLocation();
  }

  void IBusIMEContext::OnDisable(IBusInputContext *context)
  {
    //nuxDebugMsg("***IBusIMEContext::OnDisable***");
    nuxAssert(context_ == context);
    text_entry_->ime_active_ = false;
    text_entry_->ResetPreedit();
    text_entry_->QueueRefresh (true, true);
  }

  void IBusIMEContext::OnDestroy(IBusInputContext *context)
  {
    //nuxDebugMsg("***IBusIMEContext::OnDestroy***");
    nuxAssert(context_ == context);

    g_object_unref(context_);
    context_ = NULL;
  }

  void IBusIMEContext::ProcessKeyEventDone(IBusInputContext *context, GAsyncResult* res, ProcessKeyEventData *data)
  {
    //nuxDebugMsg("***IBusIMEContext::ProcessKeyEventDone***");
      nuxAssert(data->context->context_ == context);

      GError *error = NULL;
      gboolean processed = ibus_input_context_process_key_event_async_finish (
                            context,
                            res,
                            &error);

      if (error != NULL)
      {
        g_warning ("Process Key Event failed: %s.", error->message);
        g_error_free (error);
      }

      if (processed == FALSE)
      {
        data->context->text_entry_->ProcessKeyEvent(data->event.type(),
                                                    data->event.key_sym(),
                                                    data->event.flags() | IBUS_IGNORED_MASK,
                                                    data->event.character().c_str(),
                                                    0);
      }

      delete data;
  }

  std::vector<Event> IBusIMEContext::ParseIBusHotkeys(const gchar** keybindings)
  {
    std::vector<Event> hotkeys;

    for (int i = 0; keybindings && keybindings[i]; ++i)
    {
      gchar** binding = g_strsplit (keybindings[i], "+", -1);

      if (binding)
      {
        Event ev;
        ev.type = EVENT_KEY_DOWN;

        for (int j = 0; binding && binding[j]; ++j)
        {
          if (strcmp(binding[j], "Release") == 0)
          {
            ev.type = EVENT_KEY_UP;
            continue;
          }

          KeySym key = XStringToKeysym(binding[j]);
          bool is_modifier = false;

          if (key)
          {
            switch (key)
            {
              case XK_Caps_Lock:
              case XK_Shift_Lock:
              case XK_Num_Lock:
              case XK_Scroll_Lock:
                is_modifier = true;
            }
          }
          else
          {
            // Checking if the current key is a generic modifier key
            key = XStringToKeysym((std::string(binding[j]) + "_L").c_str());
            is_modifier = (key != 0);
          }

          if (is_modifier)
          {
            switch (key)
            {
              case XK_Control_L:
              case XK_Control_R:
                ev.key_modifiers |= KEY_MODIFIER_CTRL;
                break;
              case XK_Shift_L:
              case XK_Shift_R:
              case XK_Shift_Lock:
                ev.key_modifiers |= KEY_MODIFIER_SHIFT;
                break;
              case XK_Alt_L:
              case XK_Alt_R:
                ev.key_modifiers |= KEY_MODIFIER_ALT;
                break;
              case XK_Super_L:
              case XK_Super_R:
                ev.key_modifiers |= KEY_MODIFIER_SUPER;
              case XK_Caps_Lock:
                ev.key_modifiers |= KEY_MODIFIER_CAPS_LOCK;
                break;
              case XK_Num_Lock:
                ev.key_modifiers |= KEY_MODIFIER_NUMLOCK;
                break;
              case XK_Scroll_Lock:
                ev.key_modifiers |= KEY_MODIFIER_SCROLLLOCK;
                break;
              // FIXME add support to Hyper and Meta keys in nux::Event
            }
          }

          if (!is_modifier && key)
            ev.x11_keysym = key;
        }

        if (ev.x11_keysym)
        {
          hotkeys.push_back(ev);
        }

        g_strfreev(binding);
      }
    }

    return hotkeys;
  }

  void IBusIMEContext::UpdateHotkeys()
  {
    IBusConfig* conf = ibus_bus_get_config(bus_);
    GVariant* val = ibus_config_get_value(conf, "general", "hotkey/trigger");
    const gchar** keybindings = g_variant_get_strv(val, NULL);

    hotkeys_ = ParseIBusHotkeys(keybindings);

    g_variant_unref(val);
  }

  bool IBusIMEContext::IsHotkeyEvent(EventType type, unsigned long keysym, unsigned long modifiers) const
  {
    for (Event const& ev : hotkeys_)
    {
      if (ev.type == type && ev.x11_keysym == keysym)
      {
        if (type == EVENT_KEY_UP)
          return (modifiers & ev.key_modifiers);
        else
          return (ev.key_modifiers == modifiers);
      }
    }

    return false;
  }
}

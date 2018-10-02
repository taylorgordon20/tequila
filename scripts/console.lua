local module = {}

local KEYS = {
  enter = 257,
  backspace = 259,
}

local console_text = ""
local console_open = false

local switch = function(condition, true_result, false_result)
  if condition then
    return true_result
  else 
    return false_result
  end
end

function module:update_console()
  update_ui_node(
    "console_back",
    {color = switch(console_open, 0x000000AA, 0x00000000)}
  )
  update_ui_node(
    "console_text",
    {
      text = ">> " .. console_text,
      color = switch(console_open, 0xFFFFFFFF, 0xFFFFFF00)
    }
  )
end

function module:on_init()
  local window_w, window_h = table.unpack(get_window_size())
  create_ui_node(
    "console_back",
    "rect",
    {x = 0, y = window_h - 256, z = 2, width = window_w, height = 256, color = 0}
  )

  -- Create a debug information text node in the top-left of the screen.
  create_ui_node(
    "console_text",
    "text",
    {x = 0, y = window_h - 256, z = 1, color = 0xFFFFFF00, text = ">> "}
  )

  -- TEST CODE:
  -- for n in pairs(_G) do print(n) end
end

function module:on_resize(width, height)
  local window_w, window_h = table.unpack(get_window_size())
  update_ui_node(
    "console_back",
    {y = window_h - 256, width = window_w}
  )
  update_ui_node(
    "console_text",
    {y = window_h - 256}
  )
end

function module:on_key(key, scancode, action, mods)
  if key == string.byte('`') and action == 1 then
    console_open = not console_open
    self:update_console()
  elseif console_open and key == KEYS.backspace and action >= 1 then
    console_text = string.reverse(string.sub(string.reverse(console_text), 2))
    self:update_console()
  elseif console_open and key == KEYS.enter and action == 1 then
    console_text = ""
    self:update_console()
  end
  return console_open
end

function module:on_text(codepoint)
  if not console_open then
    return
  end
  if codepoint ~= string.byte('`') then
    console_text = console_text .. utf8.char(codepoint)
    self:update_console()
  end
  return console_open
end

function module:on_update(dt)
  return console_open
end

return module
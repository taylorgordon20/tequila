local module = {}

local KEYS = {
  down = 264,
  up = 265,
  enter = 257,
  backspace = 259,
}

local console_font = 0.025
local console_size = 0.9
local console_logs = {}
local console_text = ""
local console_open = false

local history = {
  index = 1,
  commands = {""},
}

function history:rewind()
  self.index = math.max(1, self.index - 1)
  console_text = self.commands[self.index]
end

function history:forward()
  self.index = math.min(#self.commands, self.index + 1)
  console_text = self.commands[self.index]
end

function history:reset()
  self.commands[#self.commands] = console_text
  self.index = #self.commands
end

function history:push(command)
  self.commands[#self.commands] = command
  table.insert(self.commands, #self.commands + 1, "")
  self.index = #self.commands
end

function module:log(line)
  table.insert(console_logs, 1, line)
  table.remove(console_logs)
end

function module:execute_command(command)
  env = {
    math = math,
    table = table,
    tostring = tostring,
    tonumber = tonumber,
    print = print,
  }

  -- Add FFI to the environment.
  for name, descriptor in pairs(__ffi) do
    env[name] = _G[name]
  end

  -- Add help function to the environment.
  env.help = function(symbol)
    if not symbol then
      print("Available symbols: ")
      symbols = {}
      for name, _ in pairs(env) do table.insert(symbols, name) end
      table.sort(symbols)
      for i, name in ipairs(symbols) do print("\t" .. name) end
    else
      value = __ffi[symbol] or env[symbol]
      print("Type of symbol '" .. symbol .. "' = " .. tostring(value))
    end
    return "Check stdout for help information"
  end

  function eval()
    return assert(load("return " .. command, nil, "t", env))()
  end

  local okay, ret = pcall(eval)
  if not okay then
    self:log("error: " .. tostring(ret))
  else
    self:log(tostring(ret))
  end
end

function module:update_console()
  local window_w, window_h = table.unpack(get_window_size())
  update_ui_node(
    "console_back",
    {
      x = 0,
      y = (1 - console_size) * window_h,
      z = #console_logs + 2,
      width = window_w,
      height = console_size * window_h,
      color = switch(console_open, 0x000000AA, 0x00000000)
    }
  )
  update_ui_node(
    "console_text",
    {
      x = 0,
      y = (1 - console_size) * window_h,
      z = 1,
      text = ">>> " .. console_text,
      color = switch(console_open, 0xFFFFFFFF, 0xFFFFFF00),
      font = math.ceil(console_font * window_h),
    }
  )
  for i = 1, #console_logs do
    line_space = math.ceil(1.5 * console_font * window_h)
    update_ui_node(
      "console_logs_" .. i,
      {
        x = 0,
        y = (1 - console_size) * window_h + i * line_space,
        z = i + 1,
        text = console_logs[i],
        color = switch(console_open, 0xFFFFFFFF, 0xFFFFFF00),
        font = math.ceil(console_font * window_h),
      }
    )
  end
end

function module:on_init()
  -- Initialize the log to empty strings.
  for i = 1, 100 do console_logs[i] = "" end

  -- Initialize the UI nodes.
  create_ui_node("console_back", "rect", {})
  create_ui_node("console_text", "text", {})
  for i = 1, #console_logs do
    create_ui_node("console_logs_" .. i, "text", {})
  end
end

function module:on_done()
  for i = 1, #console_logs do 
    delete_ui_node("console_logs_" .. i)
  end
  delete_ui_node("console_text")
  delete_ui_node("console_back")
end

function module:on_resize(width, height)
  self:update_console()
end

function module:on_key(key, scancode, action, mods)
  if key == string.byte('`') and action == 1 then
    console_open = not console_open
    self:update_console()
  elseif console_open and key == KEYS.up and action == 1 then
    history:rewind()
    self:update_console()
  elseif console_open and key == KEYS.down and action == 1 then
    history:forward()
    self:update_console()
  elseif console_open and key == KEYS.backspace and action >= 1 then
    console_text = string.sub(console_text, 1, #console_text - 1)
    history:reset()
    self:update_console()
  elseif console_open and key == KEYS.enter and action == 1 then
    local command = console_text
    self:log(">>> " .. command)
    history:push(command)
    console_text = ""
    self:execute_command(command)
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
    history:reset()
    self:update_console()
  end
  return console_open
end

function module:on_update(dt)
  return console_open
end

return module
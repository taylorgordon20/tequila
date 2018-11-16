local module = {}

local alert_show = 0
local alert_time = 0
local alert_text = ""

function module:show_alert(text)
  alert_time = now_time()
  alert_text = text
  alert_show = 20
  self:update_ui()
end

function module:update_ui()
  local window_w, window_h = table.unpack(get_window_size())
  local text_length = string.len(alert_text)
  update_ui_node(
    "alert",
    {
      x = math.floor(0.5 * window_w - 0.5 * text_length * 35),
      y = math.floor(0.5 * window_h),
      z = 1,
      text = alert_text,
      color = 0xFFAAAA00 + math.floor(0xFF * alert_show / 20.0),
      size = 72,
      font = "Old_Standard_TT/OldStandard-Regular",
    }
  )
end

function module:on_init()
  create_ui_node(
    "alert",
    "text",
    {x = 0, y = 0, color = 0xFFFFFFFF, text = "", size = 72}
  )

  self:show_alert("Welcome to my game!")
end

function module:on_done()
  delete_ui_node("alert")
end

function module:on_update(dt)
  -- Update the text alpha every second.
  if alert_show > 0 then
    local alert_secs = now_time() - alert_time
    if alert_secs > 0.5 then
      alert_show = alert_show - 1
      self:update_ui()
      alert_time = now_time()
    end
  end

  -- Check to see if the player found the secret spot.
  local cx, cy, cz = table.unpack(get_camera_pos())
  local x_dist = math.abs(cx - 298.5)
  local y_dist = math.abs(cy - 33.5)
  local z_dist = math.abs(cz - 222.5)
  if x_dist < 2 and y_dist < 2 and z_dist < 2 then
    self:show_alert("You found the secret!")
  end
end

return module
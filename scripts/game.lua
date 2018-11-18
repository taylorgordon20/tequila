local module = {}

local alert_show = 0
local alert_time = 0
local alert_text = ""

local secret_1 = false
local secret_2 = false

function module:camera_is_at(x, y, z)
  local cx, cy, cz = table.unpack(get_camera_pos())
  return math.abs(cx - x) < 1 and math.abs(cy - y) < 1 and math.abs(cz - z) < 1
end

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
      x = math.floor(0.5 * window_w - 0.5 * text_length * 30),
      y = math.floor(0.5 * window_h),
      z = 1,
      text = alert_text,
      color = 0xFFAAAA00 + math.floor(0xFF * alert_show / 20.0),
      size = 72,
      font = "Old_Standard_TT/OldStandard-Regular",
    }
  )
  update_ui_node(
    "secret_flag",
    {
      x = math.floor(window_w - 280),
      y = math.floor(window_h - 50),
      z = 1,
      text = "Secret Found: " .. switch(secret_2, "Yes", "No"),
      color = 0xFFFF66FF,
      size = 32,
      font = "Ubuntu/Ubuntu-Bold",
    }
  )
end

function module:on_init()
  create_ui_node(
    "alert",
    "text",
    {x = 0, y = 0, color = 0xFFFFFFFF, text = "", size = 72}
  )

  create_ui_node(
    "secret_flag",
    "text",
    {x = 0, y = 0, color = 0xFFFFFFFF, text = "", size = 32}
  )

  self:show_alert("Try to find the secret...")
end

function module:on_done()
  delete_ui_node("secret_flag")
  delete_ui_node("alert")
end

function module:on_resize(width, height)
  self:update_ui()
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
  if not secret_1 and self:camera_is_at(265.5, 37.5, 223.5) then
    secret_1 = true
    self:show_alert("Hmm... what's in here?")
  elseif not secret_2 and self:camera_is_at(299.5, 33.5, 222.5) then 
    secret_2 = true
    self:show_alert("You found the secret!")
    
    -- Build canon.
    local black_brick = 23
    set_voxel(316, 30, 221, black_brick)
    set_voxel(316, 30, 222, black_brick)
    set_voxel(316, 30, 223, black_brick)
    set_voxel(317, 30, 221, black_brick)
    set_voxel(317, 29, 222, black_brick)
    set_voxel(317, 30, 223, black_brick)
    set_voxel(318, 30, 221, black_brick)
    set_voxel(318, 30, 222, black_brick)
    set_voxel(318, 30, 223, black_brick)
  elseif secret_2 and self:camera_is_at(317.5, 31, 222.5) then
    get_module("camera"):set_velocity(0, 100, 0)
  elseif self:camera_is_at(261.5, 82, 248.5) then
    get_module("camera"):set_velocity(-800, 50, 800)
  elseif self:camera_is_at(261.5, 82, 245.5) then
    get_module("camera"):set_velocity(-800, 50, -800)
  end
end

return module
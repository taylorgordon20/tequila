local module = {}

local dt_mean = 0
local delay_s = 0

function module:on_init()
  create_ui_node(
    "camera",
    "text",
    {x = 10, y = 40, color = 0xFFFFFFFF, text = "Pos: ..."}
  )
  create_ui_node(
    "fps",
    "text",
    {x = 10, y = 10, color = 0xFFFFFFFF, text = "FPS: ..."}
  )
end

function module:on_done()
  delete_ui_node("camera")
  delete_ui_node("fps")
end

function module:on_update(dt)
  -- Update FPS counter.
  dt_mean = 0.1 * dt + 0.9 * dt_mean

  -- Update the debug UI every second.
  delay_s = delay_s + dt
  if delay_s > 1.0 then
    local cx, cy, cz = table.unpack(get_camera_pos())
    update_ui_node(
      "camera",
      {text = string.format("Pos: %.2f, %.2f, %.2f", cx, cy, cz)}
    )
    update_ui_node(
      "fps",
      {text = string.format("FPS: %.2f", 1 / dt_mean)}
    )
    delay_s = 0.0
  end
end

return module
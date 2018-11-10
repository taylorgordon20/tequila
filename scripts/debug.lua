local module = {}

local dt_mean = 0
local delay_s = 0
local day_night = {
  enabled = false,
  angle = 0,
  delay = 0,
}

function module:on_init()
  create_ui_node(
    "camera",
    "text",
    {x = 10, y = 25, color = 0xFFFFFFFF, text = "Pos: ...", size = 16}
  )
  create_ui_node(
    "fps",
    "text",
    {x = 10, y = 5, color = 0xFFFFFFFF, text = "FPS: ...", size = 16}
  )
  create_ui_node(
    "terrain_slices",
    "text",
    {x = 10, y = 45, color = 0xFFFFFFFF, text = "Terrain Slices: ...", size = 16}
  )

  -- Registry a console command to set palette styles.
  get_module("console"):create_command(
      "print_stats",
      function()
        local stats = get_stats()
        local stat_keys = {}
        for index, key in pairs(stats) do
          table.insert(stat_keys, key)
        end
        table.sort(stat_keys)
        for index, key in ipairs(stat_keys) do
          local avg = get_stat_average(key)
          local line =  string.format("%s\tavg: %.6f", key, avg)
          get_module("console"):log(line)
        end
      end
  )

  get_module("console"):create_command(
      "toggle_day_night",
      function()
        day_night.enabled = not day_night.enabled
      end
  )
end

function module:on_done()
  -- Unregister console commands.
  get_module("console"):delete_command("toggle_day_night")
  get_module("console"):delete_command("set_style")

  delete_ui_node("terrain_slices")
  delete_ui_node("fps")
  delete_ui_node("camera")
end

function module:on_update(dt)
  -- Update FPS counter.
  dt_mean = 0.01 * dt + 0.99 * dt_mean

  -- Update the debug UI every second.
  delay_s = delay_s + dt
  if delay_s > 1.0 then
    local cx, cy, cz = table.unpack(get_camera_pos())
    update_ui_node(
      "camera",
      {text = string.format("Pos: %.2f, %.2f, %.2f", cx, cy, cz)}
    )

    local fps = 1 / dt_mean
    update_ui_node(
      "fps",
      {text = string.format("FPS: %.2f", fps)}
    )

    local terrain_slices = get_stat_average("terrain_slices_count") or 0
    update_ui_node(
      "terrain_slices",
      {text = string.format("Terrain Meshes: %.2f", terrain_slices)}
    )
    delay_s = 0.0
  end

  -- Simulate day-night.
  day_night.delay = day_night.delay + dt
  if day_night.enabled and day_night.delay > 10.0 then
    day_night.angle = day_night.angle + 0.1
    x = math.cos(day_night.angle)
    z = math.cos(day_night.angle)
    y = math.sin(day_night.angle)
    set_light_dir(x, y, z)
    day_night.delay = 0
  end
end

return module
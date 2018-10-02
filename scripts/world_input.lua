local module = {
  orientation_toggle = true,
  orientation_angles = {0, 0},
  physics_mode = false,
  camera_velocity = {0, 0, 0},
  crosshair_size = 0.005,
  crosshair_color = 0xAAAAFFCC,
  palette_colors = {},
  palette_selection = 1,
  edit_delay_s = 0,
  debug_dt_avg = 0.0167,
  debug_delay_s = 0,
}

local KEYS = {
  space = 32,
  tab = 258,
  pg_up = 266,
  pg_dn = 267,
  home = 268,
  left_shift = 340,
}

local PHYSICS = {
  resistance_force = {-0.5, -0.001, -0.5},
  gravity_force = -0.4,
  jump_force = 12.0,
  walk_force = 10.0,
  run_force = 50.0,
}

local PI = 2 * math.asin(1)

local switch = function(condition, true_result, false_result)
  if condition then
    return true_result
  else 
    return false_result
  end
end

local clamp = function(val, min, max)
  return switch(val < min, min, switch(val > max, max, val))
end

local clamp_abs = function(val, size)
  return clamp(val, -math.abs(size), math.abs(size))
end

local truncate = function(val, threshold)
  return switch(math.abs(val) <= threshold, 0, val)
end

local for_camera_ray_voxels = function(voxel_fn)
  local cx, cy, cz = table.unpack(get_camera_pos())
  local vx, vy, vz = table.unpack(get_camera_view())
  local voxels = get_ray_voxels(cx, cy, cz, vx, vy, vz, 20)
  for i, voxel_coord in ipairs(voxels) do
    local x, y, z = table.unpack(voxel_coord)
    local dx = math.max(0, math.abs(cx - x - 0.5) - 0.5)
    local dy = math.max(0, math.abs(cy - y - 0.5) - 0.5)
    local dz = math.max(0, math.abs(cz - z - 0.5) - 0.5)
    local distance = math.sqrt(dx * dx + dy * dy + dz * dz)
    if voxel_fn(x, y, z, distance) then
      break
    end
  end
end

local clamp_movement = function(from, move, dim)
  local pad = 0.001
  local p = from[dim]
  local m = move[dim]
  local s = {0, 0, 0}
  if dim == 1 then
    s = {switch(m < 0, -1, 1), 0, 0}
  elseif dim == 2 then
    s = {move[1], switch(m < 0, -1, 1), 0}
  else
    s = {move[1], move[2], switch(m < 0, -1, 1)}
  end
  if get_voxel(from[1] + s[1], from[2] + s[2], from[3] + s[3]) ~= 0 then
    move[dim] = clamp(m, math.floor(p) - p + pad, math.floor(p + 1) - p - pad)
  end
end

local random_color = function()
  local r = math.random(0, 255) << 24;
  local g = math.random(0, 255) << 16;
  local b = math.random(0, 255) << 8;
  return r + g + b + 255;
end

function module:get_flying_movement(dt)
  local move = {0, 0, 0}
  local force = switch(
      is_key_pressed(KEYS.left_shift),
      PHYSICS.run_force,
      PHYSICS.walk_force
  )
  local view_x, view_y, view_z = table.unpack(get_camera_view())
  if is_key_pressed(string.byte('W')) then
    move[1] = move[1] + force * dt * view_x
    move[2] = move[2] + force * dt * view_y
    move[3] = move[3] + force * dt * view_z
  end
  if is_key_pressed(string.byte('S')) then
    move[1] = move[1] - force * dt * view_x
    move[2] = move[2] - force * dt * view_y
    move[3] = move[3] - force * dt * view_z
  end
  if is_key_pressed(string.byte('A')) then
    move[1] = move[1] + force * dt * view_z
    move[3] = move[3] - force * dt * view_x
  end
  if is_key_pressed(string.byte('D')) then
    move[1] = move[1] - force * dt * view_z
    move[3] = move[3] + force * dt * view_x
  end
  if is_key_pressed(KEYS.pg_up) then
    move[2] = move[1] + force * dt
  end
  if is_key_pressed(KEYS.pg_dn) then
    move[2] = move[1] - force * dt
  end
  return move
end

function module:get_physics_movement(dt)
  -- Start with the inertial velocity.
  local move = {
    dt * self.camera_velocity[1],
    dt * self.camera_velocity[2],
    dt * self.camera_velocity[3],
  }

  -- Apply a force of keyboard-induced self-propulsion.
  local orient_x = math.sin(self.orientation_angles[1])
  local orient_z = math.cos(self.orientation_angles[1])
  local propulsion = switch(
      is_key_pressed(KEYS.left_shift),
      PHYSICS.run_force,
      PHYSICS.walk_force
  )
  if is_key_pressed(string.byte('W')) then
    move[1] = move[1] + dt * propulsion * orient_x
    move[3] = move[3] + dt * propulsion * orient_z
  end
  if is_key_pressed(string.byte('S')) then
    move[1] = move[1] - dt * propulsion * orient_x
    move[3] = move[3] - dt * propulsion * orient_z
  end
  if is_key_pressed(string.byte('A')) then
    move[1] = move[1] + dt * propulsion * orient_z
    move[3] = move[3] - dt * propulsion * orient_x
  end
  if is_key_pressed(string.byte('D')) then
    move[1] = move[1] - dt * propulsion * orient_z
    move[3] = move[3] + dt * propulsion * orient_x
  end

  -- Apply a gravity force.
  move[2] = move[2] + dt * PHYSICS.gravity_force

  -- Add the resistance force.
  move[1] = (1 + PHYSICS.resistance_force[1]) * move[1]
  move[2] = (1 + PHYSICS.resistance_force[2]) * move[2]
  move[3] = (1 + PHYSICS.resistance_force[3]) * move[3]

  -- Truncate the movement vector to zero below some threshold.
  move[1] = truncate(move[1], 0.00001)
  move[2] = truncate(move[2], 0.00001)
  move[3] = truncate(move[3], 0.00001)

  -- Clamp the movement vector to avoid collisions.
  -- TODO: Tweak the ranges below to be parameterized on player dimensions.
  local cx, cy, cz = table.unpack(get_camera_pos())
  for dim = 1, 3 do
    for _, dx in ipairs({-0.40, 0.40}) do
      for _, dy in ipairs({-1.6, -0.65, 0.3}) do
        for _, dz in ipairs({-0.40, 0.40}) do
          clamp_movement({cx + dx, cy + dy, cz + dz}, move, dim)
        end
      end
    end
  end

  -- Record the inertial velocity.
  self.camera_velocity[1] = move[1] / dt
  self.camera_velocity[2] = move[2] / dt
  self.camera_velocity[3] = move[3] / dt

  return move
end

function module:update_ui()
  local window_w, window_h = table.unpack(get_window_size())

  -- Position the crosshair at the center of the screen.
  local crosshair_size = math.ceil(self.crosshair_size * window_h)
  update_ui_node(
      "crosshair",
      {
        x = window_w / 2 - crosshair_size / 2,
        y = window_h / 2 - crosshair_size / 2,
        width = crosshair_size,
        height = crosshair_size,
        color = self.crosshair_color,
      }
  )

  -- Position the color palette centered in the bottom of the screen.
  local palette_size = window_h * 0.05
  local palette_padding = window_h * 0.02
  local palette_shift = (window_w - 8 * (palette_size + palette_padding)) / 2
  local palette_x = function(index)
    return index * palette_padding + (index - 1) * palette_size + palette_shift
  end
  for i = 1, 8 do
    local alpha = switch(i == self.palette_selection, 255, 100)    
    update_ui_node(
        "palette_" .. i,
        {          
          x = palette_x(i),
          y = palette_padding,
          z = 1,
          width = palette_size,
          height = palette_size,
          color = (self.palette_colors[i] & ~255) + alpha,
        }
    )    
  end

  -- Position the selection highlight beneath the appropriate palette.
  local selection_padding = window_h * 0.01
  update_ui_node(
    "palette_selection",
    {
      x = palette_x(self.palette_selection) - selection_padding / 2,
      y = palette_padding - selection_padding / 2,
      z = 2,
      width = palette_size + selection_padding,
      height = palette_size + selection_padding,
      color = 0xFFFFFF77,
    }
  )
end

function module:insert_voxel()
  local pred = nil
  for_camera_ray_voxels(function(x, y, z, distance)
    if get_voxel(x, y, z) ~= 0 then
      if pred then
        local color = self.palette_colors[self.palette_selection]
        set_voxel(pred[1], pred[2], pred[3], color)
        self.edit_delay_s = 0
      end
      return true
    end
    if distance > 0.2 then
      pred = {x, y, z}
    end
  end)
end

function module:remove_voxel()
  for_camera_ray_voxels(function(x, y, z)
    if get_voxel(x, y, z) ~= 0 then
      set_voxel(x, y, z, 0)
      self.edit_delay_s = 0
      return true
    end
  end)
end

function module:on_init()
  print("Initialized world_input.lua")
  set_cursor_visible(false)

  -- Create a crosshair that remains centered in the screen.
  create_ui_node(
      "crosshair",
      "rect",
      {x = 0, y = 0, width = 0, height = 0, color = 0}
  )

  -- Assign the initial palette colors and create the UI.
  for i = 1, 8 do
    self.palette_colors[i] = random_color()
    create_ui_node(
        "palette_" .. i,
        "rect",
        {x = 0, y = 0, width = 0, height = 0, color = 0}
    )
  end
  create_ui_node(
    "palette_selection",
    "rect",
    {x = 0, y = 0, width = 0, height = 0, color = 0}
  )
  self.palette_selection = 1

  -- Create a debug information text node in the top-left of the screen.
  create_ui_node(
    "debug_info",
    "text",
    {x = 10, y = 10, color = 0xFFFFFFFF, text = "FPS: calculating..."}
  )

  -- Update the UI relative to the current window size.
  self:update_ui()

  -- TEST CODE:
  for n in pairs(_G) do print(n) end
end

function module:on_resize(width, height)
  self:update_ui()
end

function module:on_scroll(x_offset, y_offset)
  self.palette_selection = math.floor(self.palette_selection - 1 - y_offset) % 8 + 1
  self:update_ui()
end

function module:on_key(key, scancode, action, mods)
  if key == string.byte('E') and action == 1 then
    self.orientation_toggle = not self.orientation_toggle
    set_cursor_visible(not self.orientation_toggle)
  elseif key == string.byte('G') and action == 1 then
    self.physics_mode = not self.physics_mode
    self.camera_velocity = {0, 0, 0}
  elseif key == KEYS.home and action == 1 then
    self.orientation_angles = {0, 0}
    set_camera_view(0, 0, 1)
  elseif key == KEYS.space and action == 1 then
    self.camera_velocity = {0, PHYSICS.jump_force, 0}
  elseif key == KEYS.tab and action == 1 then
    self.palette_selection = self.palette_selection % 8 + 1
    self:update_ui()
  end
end

function module:on_click(button, action, mods)
  if self.orientation_toggle then
    if button == 0 and action == 1 then
      self:insert_voxel()
    elseif button == 1 and action == 1 then
      self:remove_voxel()
    end
  end
end

function module:on_update(dt)
  local camera_move = {0, 0, 0}
  if self.physics_mode then
    camera_move = self:get_physics_movement(dt)
  else
    camera_move = self:get_flying_movement(dt)
  end

  if camera_move[1] ~= 0 or camera_move[2] ~= 0 or camera_move[2] ~= 0 then
    local cx, cy, cz = table.unpack(get_camera_pos())
    local mx, my, mz = table.unpack(camera_move)
    set_camera_pos(cx + mx, cy + my, cz + mz)
  end

  -- Update the camera orientation as the cursor moves.
  if self.orientation_toggle then
    local mx, my = table.unpack(get_cursor_pos())
    local ww, wh = table.unpack(get_window_size())

    -- Update the camera orientation based on cursor movement.
    local speed = 0.1
    local theta_phi = self.orientation_angles
    theta_phi[1] = theta_phi[1] + speed * dt * math.floor(0.5 * ww - mx)
    theta_phi[2] = theta_phi[2] + speed * dt * math.floor(0.5 * wh - my)
    theta_phi[2] = clamp(theta_phi[2], -0.48 * PI, 0.48 * PI)
    set_camera_view(
      math.sin(theta_phi[1]) * math.cos(theta_phi[2]),
      math.sin(theta_phi[2]),
      math.cos(theta_phi[1]) * math.cos(theta_phi[2])
    )

    -- Reset the cursor back to the center of the screen.
    set_cursor_pos(0.5 * ww, 0.5 * wh)

    -- Applied delay voxel edits.
    self.edit_delay_s = self.edit_delay_s + dt
    if is_mouse_pressed(0) and self.edit_delay_s > 0.15 then
      self:insert_voxel()
    elseif is_mouse_pressed(1) and self.edit_delay_s > 0.15 then
      self:remove_voxel()
    end
  end

  -- Calculate the frame rate statistics and render into the debug panel.
  self.debug_dt_avg = 0.1 * dt + 0.9 * self.debug_dt_avg
  self.debug_delay_s = self.debug_delay_s + dt
  if self.debug_delay_s > 1.0 then
    update_ui_node(
      "debug_info",
      {text = string.format("FPS: %.2f", 1 / self.debug_dt_avg)}
    )
    self.debug_delay_s = 0.0
  end
end

return module
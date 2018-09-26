local module = {
  orientation_toggle = true,
  orientation_angles = {0, 0},
  physics_mode = false,
  physics_jump = 0,
}

local KEYS = {
  space = 32,
  pg_up = 266,
  pg_dn = 267,
  home = 268,
  left_shift = 340,
}

local PHYSICS = {
  gravity = -0.5,
  terminal_speed = 50.0,
  jump_speed = 15.0,
}

local PI = 2 * math.asin(1)

local clamp = function(val, min, max)
  return val < min and min or val > max and max or val
end

local clamp_abs = function(val, size)
  return clamp(val, -size, size)
end

local handle_key_movement = function(dt, move)
  local speed = is_key_pressed(KEYS.left_shift) and 50.0 or 10.0
  local view_x, view_y, view_z = table.unpack(get_camera_view())
  if is_key_pressed(string.byte('W')) then
    move[1] = move[1] + speed * dt * view_x
    move[2] = move[2] + speed * dt * view_y
    move[3] = move[3] + speed * dt * view_z
  end
  if is_key_pressed(string.byte('S')) then
    move[1] = move[1] - speed * dt * view_x
    move[2] = move[2] - speed * dt * view_y
    move[3] = move[3] - speed * dt * view_z
  end
  if is_key_pressed(string.byte('A')) then
    move[1] = move[1] + speed * dt * view_z
    move[3] = move[3] - speed * dt * view_x
  end
  if is_key_pressed(string.byte('D')) then
    move[1] = move[1] - speed * dt * view_z
    move[3] = move[3] + speed * dt * view_x
  end
  if is_key_pressed(KEYS.pg_up) then
    move[2] = move[1] + speed * dt
  end
  if is_key_pressed(KEYS.pg_dn) then
    move[2] = move[1] - speed * dt
  end
end

local clamp_movement = function(move)
  local x, y, z = table.unpack(get_camera_pos())
  local fx = x - math.floor(x)
  local fy = y - math.floor(y)
  local fz = z - math.floor(z)
  local lv = get_voxel(x - 1, y, z) ~= 0 and 1 or 0
  local rv = get_voxel(x + 1, y, z) ~= 0 and 1 or 0
  local dv = get_voxel(x, y - 1, z) ~= 0 and 1 or 0
  local uv = get_voxel(x, y + 1, z) ~= 0 and 1 or 0
  local bv = get_voxel(x, y, z - 1) ~= 0 and 1 or 0
  local fv = get_voxel(x, y, z + 1) ~= 0 and 1 or 0
  local px, py, pz = table.unpack({0.4, 0.8, 0.4})
  move[1] = clamp(move[1], lv * (1 - fx + px) - 1, 1 - rv * (fx + px))
  move[2] = clamp(move[2], dv * (1 - fy + py) - 1, 1 - uv * (fy + py))
  move[3] = clamp(move[3], bv * (1 - fz + pz) - 1, 1 - fv * (fz + pz))
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

function module:on_init()
  print("Initialized world_input.lua")
  set_cursor_visible(false)
end

function module:on_key(key, scancode, action, mods)
  if key == string.byte('E') and action == 1 then
    self.orientation_toggle = not self.orientation_toggle
    set_cursor_visible(not self.orientation_toggle)
  elseif key == string.byte('G') and action == 1 then
    self.physics_mode = not self.physics_mode
    self.physics_jump = 0
  elseif key == KEYS.home and action == 1 then
    self.orientation_angles = {0, 0}
    set_camera_view(0, 0, 1)
  elseif key == KEYS.space and action == 1 then
    self.physics_jump = PHYSICS.jump_speed
  end
end

function module:on_click(button, action, mods)
  if button == 0 and action == 1 then
    -- Insert a voxel preceding the camera ray intersection.
    local pred = nil
    for_camera_ray_voxels(function(x, y, z, distance)
      if get_voxel(x, y, z) ~= 0 then
        if pred then
          set_voxel(pred[1], pred[2], pred[3], 0xFFFFFFFF)
        end
        return true
      end
      if distance > 0.2 then
        pred = {x, y, z}
      end
    end)
  elseif button == 1 and action == 1 then
    -- Delete the voxel at the first camera ray intersection.
    for_camera_ray_voxels(function(x, y, z)
      if get_voxel(x, y, z) ~= 0 then
        set_voxel(x, y, z, 0)
        return true
      end
    end)
  end
end

function module:on_update(dt)
  local camera_move = {0, 0, 0}

  -- Update the camera move vector based on key input.
  handle_key_movement(dt, camera_move)

  -- Update the camera move vector based on phyics.
  if self.physics_mode then
    -- Apply the current jump velocity to the movement vector.
    self.physics_jump = self.physics_jump + PHYSICS.gravity
    self.physics_jump = clamp_abs(self.physics_jump, PHYSICS.terminal_speed)
    camera_move[2] = camera_move[2] + dt * self.physics_jump

    -- Clamp the movement vector to avoid collisions.
    clamp_movement(camera_move)
  end

  if camera_move[1] ~= 0 or camera_move[2] ~= 0 or camera_move[2] ~= 0 then
    local cx, cy, cz = table.unpack(get_camera_pos())
    local vx, vy, vz = table.unpack(camera_move)
    set_camera_pos(cx + vx, cy + vy, cz + vz)
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
  end
end

return module
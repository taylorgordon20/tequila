local module = {}

local orientation_toggle = true
local orientation_angles = {0, 0}

local physics = {
  enabled = false,
  velocity = {0, 0, 0},
  resistance_force = {-0.2, -0.001, -0.2},
  gravity_force = -0.4,
  jump_force = 10.0,
  walk_force = 2.0,
  run_force = 5.0,
  fly_force = 4.0,
  extra_jump = true,
  extra_jump_enabled = false,
}

function module:toggle_extra_jump()
  physics.extra_jump_enabled = not physics.extra_jump_enabled
end

function module:set_velocity(x, y, z)
  physics.velocity = {x, y, z}
end

function module:clamp_movement(from, move, dim)
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

function module:get_flying_movement(dt)
  local move = {0, 0, 0}
  local force = physics.fly_force * switch(
      is_key_pressed(KEYS.left_shift),
      physics.run_force,
      physics.walk_force
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
    dt * physics.velocity[1],
    dt * physics.velocity[2],
    dt * physics.velocity[3],
  }

  -- Apply a force of keyboard-induced self-propulsion.
  local orient_x = math.sin(orientation_angles[1])
  local orient_z = math.cos(orientation_angles[1])
  local propulsion = switch(
      is_key_pressed(KEYS.left_shift),
      physics.run_force,
      physics.walk_force
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
  if is_key_pressed(KEYS.space) then
    if physics.velocity[2] == 0 then
      move[2] = dt * physics.jump_force
    elseif physics.extra_jump_enabled and physics.extra_jump then
      move[2] = dt * physics.jump_force
      physics.extra_jump = false
    end
  end

  -- Apply a gravity force.
  move[2] = move[2] + dt * physics.gravity_force

  -- Add the resistance force.
  move[1] = (1 + physics.resistance_force[1]) * move[1]
  move[2] = (1 + physics.resistance_force[2]) * move[2]
  move[3] = (1 + physics.resistance_force[3]) * move[3]

  -- Clamp the movement vector to avoid collisions.
  -- TODO: Tweak the ranges below to be parameterized on player dimensions.
  local cx, cy, cz = table.unpack(get_camera_pos())
  for dim = 1, 3 do
    for _, dx in ipairs({-0.40, 0.40}) do
      for _, dy in ipairs({-1.6, -0.65, 0.3}) do
        for _, dz in ipairs({-0.40, 0.40}) do
          self:clamp_movement({cx + dx, cy + dy, cz + dz}, move, dim)
        end
      end
    end
  end

  -- Truncate the movement vector to zero below some threshold.
  move[1] = truncate(move[1], 0.00001)
  move[2] = truncate(move[2], 0.00001)
  move[3] = truncate(move[3], 0.00001)

  -- Reset extra jump if camera is on ground.
  if move[2] == 0 then
    physics.extra_jump = true
  end

  -- Record the inertial velocity.
  physics.velocity[1] = move[1] / dt
  physics.velocity[2] = move[2] / dt
  physics.velocity[3] = move[3] / dt

  return move
end

function module:enable_orientation(enable)
  orientation_toggle = enable
  show_cursor(not enable)
end

function module:enable_physics(enable)
  physics.enabled = enable
  physics.velocity = {0, 0, 0}
end

function module:update_camera_position(move_vector)
  local cx, cy, cz = table.unpack(get_camera_pos())
  local mx, my, mz = table.unpack(move_vector)
  set_camera_pos(cx + mx, cy + my, cz + mz)
end

function module:update_camera_view()
  local theta, phi = table.unpack(orientation_angles)
  set_camera_view(
    math.sin(theta) * math.cos(phi),
    math.sin(phi),
    math.cos(theta) * math.cos(phi)
  )
end

function module:on_init()
  self:enable_orientation(true)
  self:enable_physics(false)
end

function module:on_done()
  self:enable_physics(false)
  self:enable_orientation(false)
end

function module:on_key(key, scancode, action, mods)
  if key == string.byte('E') and action == 1 then
    self:enable_orientation(not orientation_toggle)
  elseif key == string.byte('G') and action == 1 then
    self:enable_physics(not physics.enabled)
  elseif key == KEYS.home and action == 1 then
    orientation_angles = {0, 0}
    self:update_camera_view()
  end
end

function module:on_update(dt)
  local camera_move = switch(
    physics.enabled,
    self:get_physics_movement(dt),
    self:get_flying_movement(dt)
  )

  -- Move the camera along the movement vector.
  if camera_move[1] ~= 0 or camera_move[2] ~= 0 or camera_move[3] ~= 0 then
    self:update_camera_position(camera_move)
  end

  -- Update the camera orientation as the cursor moves.
  if orientation_toggle then
    local mx, my = table.unpack(get_cursor_pos())
    local ww, wh = table.unpack(get_window_size())

    -- Update the camera orientation based on cursor movement.
    local speed = 0.1 * dt
    local theta, phi = table.unpack(orientation_angles)
    local delta_x = math.floor(0.5 * ww - mx)
    local delta_y = math.floor(0.5 * wh - my)
    if math.abs(delta_x) > 0 or math.abs(delta_y) > 0 then
      orientation_angles = {
        theta + speed * delta_x,
        clamp(phi + speed * delta_y, -0.48 * PI, 0.48 * PI),
      }
      self:update_camera_view()
    end

    -- Reset the cursor back to the center of the screen.
    set_cursor_pos(0.5 * ww, 0.5 * wh)
  end
end

return module
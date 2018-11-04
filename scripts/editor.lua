local module = {
  palette_styles = {},
  palette_selection = 1,
  palette_count = 10,
}

local crosshair_size = 0.005
local crosshair_color = 0xAAAAFFCC
local edit_delay_s = 0

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

function module:set_palette_index(index)
  self.palette_selection = index
  self:update_ui()
  return true
end

function module:set_palette_style(index, style)
  self.palette_styles[index] = style
  self:update_ui()
  return true
end

function module:update_ui()
  local window_w, window_h = table.unpack(get_window_size())

  -- Position the crosshair at the center of the screen.
  local crosshair_pix = math.ceil(crosshair_size * window_h)
  update_ui_node(
      "crosshair",
      {
        x = window_w / 2 - crosshair_size / 2,
        y = window_h / 2 - crosshair_size / 2,
        width = crosshair_pix,
        height = crosshair_pix,
        color = crosshair_color,
      }
  )

  -- Position the style palette centered in the bottom of the screen.
  local palette_size = window_h * 0.05
  local palette_padding = window_h * 0.02
  local palette_shift = (window_w - self.palette_count * (palette_size + palette_padding)) / 2
  local palette_x = function(index)
    return index * palette_padding + (index - 1) * palette_size + palette_shift
  end
  for i = 1, #self.palette_styles do
    local alpha = switch(i == self.palette_selection, 0xFF, 0xAA)    
    update_ui_node(
        "palette_" .. i,
        {          
          x = palette_x(i),
          y = palette_padding,
          z = 1,
          width = palette_size,
          height = palette_size,
          color = 0xffffff00 + alpha,
          style = self.palette_styles[i]
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
        local style = self.palette_styles[self.palette_selection]
        set_voxel(pred[1], pred[2], pred[3], style)
        edit_delay_s = 0
      end
      return true
    end
    if distance > 1.0 then
      pred = {x, y, z}
    end
  end)
end

function module:remove_voxel()
  for_camera_ray_voxels(function(x, y, z)
    if get_voxel(x, y, z) ~= 0 then
      set_voxel(x, y, z, 0)
      edit_delay_s = 0
      return true
    end
  end)
end

function module:on_init()
  -- Registry a console command to set palette styles.
  get_module("console"):create_command(
      "set_style",
      function(index, style)
        self:set_palette_style(index, style)
      end
  )

  -- Create a crosshair that remains centered in the screen.
  create_ui_node("crosshair", "rect", {})

  -- Assign the initial palette styles and create the UI.
  for i = 1, self.palette_count do
    self.palette_styles[i] = i + 1
    create_ui_node("palette_" .. i, "style", {})
  end
  create_ui_node("palette_selection", "rect", {})
  self.palette_selection = 1

  -- Update the UI relative to the current window size.
  self:update_ui()
end

function module:on_done()
  delete_ui_node("palette_selection")
  for i = 1, #self.palette_styles do
    delete_ui_node("palette_" .. i)
  end
  delete_ui_node("crosshair")

  -- Unregister console commands.
  get_module("console"):delete_command("set_style")
end

function module:on_resize(width, height)
  self:update_ui()
end

function module:on_scroll(x_offset, y_offset)
  self.palette_selection = math.floor(self.palette_selection - 1 - y_offset) % self.palette_count + 1
  self:update_ui()
end

function module:on_key(key, scancode, action, mods)
  if key == KEYS.tab and action == 1 then
    self.palette_selection = self.palette_selection % self.palette_count + 1
    self:update_ui()
  end
end

function module:on_click(button, action, mods)
  if button == 0 and action == 1 then
    self:insert_voxel()
  elseif button == 1 and action == 1 then
    self:remove_voxel()
  end
end

function module:on_update(dt)
  -- Apply delayed voxel edits.
  edit_delay_s = edit_delay_s + dt
  if is_mouse_pressed(0) and edit_delay_s > 0.25 then
    self:insert_voxel()
  elseif is_mouse_pressed(1) and edit_delay_s > 0.25 then
    self:remove_voxel()
  end
end

return module
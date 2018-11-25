local module = {
  palette_styles = {},
  palette_selection = 1,
  palette_count = 16,
}

local crosshair_size = 0.005
local crosshair_color = 0xAAAAFFCC
local edit_delay_s = 0
local box_edit_start = nil

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

  -- Display the current state of the box edit mode.
  if box_edit_start then
    local be_x, be_y, be_z = table.unpack(box_edit_start)
    update_ui_node(
      "box_edit",
      {
        x = window_w - 180,
        text = string.format("Box edit: %.0f, %.0f, %.0f", be_x, be_y, be_z),
      }
    )
  else
    update_ui_node(
      "box_edit",
      {x = window_w - 180, text = "Box edit: nil"}
    )
  end


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

function module:get_ray_insertion_voxel()
  local ret = nil
  local pred = nil
  for_camera_ray_voxels(function(x, y, z, distance)
    if get_voxel(x, y, z) ~= 0 then
      if pred then
        local cam_x, cam_y, cam_z = table.unpack(get_camera_pos())
        local px, py, pz = table.unpack(pred)
        local dx = math.abs(px - cam_x + 0.5)
        local dy = py - cam_y + 0.5
        local dz = math.abs(pz - cam_z + 0.5)
        if dx > 0.9 or dz > 0.9 or dy < -2.1 or dy > 0.8 then
          ret = {px, py, pz}
        end
      end
      return true
    end
    pred = {x, y, z}
  end)
  return ret
end

function module:insert_voxel()
  if edit_delay_s < 0.3 then
    return
  end

  local voxel = self:get_ray_insertion_voxel()
  if voxel then
    local x, y, z = table.unpack(voxel)
    local style = self.palette_styles[self.palette_selection]
    set_voxel(x, y, z, style)
    edit_delay_s = 0
  end
end

function module:remove_voxel()
  if edit_delay_s < 0.3 then
    return
  end

  for_camera_ray_voxels(function(x, y, z)
    if get_voxel(x, y, z) ~= 0 then
      set_voxel(x, y, z, 0)
      edit_delay_s = 0
      return true
    end
  end)
end

function module:insert_voxel_box(from, to, style, override)
  local sx, sy, sz = table.unpack(from)
  local ex, ey, ez = table.unpack(to)

  -- Swap indices so that from increases towards to.
  if sx > ex then
    sx, ex = ex, sx
  end
  if sy > ey then
    sy, ey = ey, sy
  end
  if sz > ez then
    sz, ez = ez, sz
  end
  
  -- Determine all insertions and set them in a batch update.
  local voxels = {}
  for iz = sz, ez do
    for iy = sy, ey do
      for ix = sx, ex do
        if override or get_voxel(ix, iy, iz) == 0 then
          voxels[{ix, iy, iz}] = style
        end
      end
    end
  end
  set_voxels(voxels)
end

function module:on_init()
  -- Registry a console command to set palette styles.
  get_module("console"):create_command(
      "set_style",
      function(index, style)
        self:set_palette_style(index, style)
      end
  )

  -- Edit indicator UI.
  create_ui_node(
    "box_edit",
    "text",
    {x = 0, y = 10, color = 0xFFFFFFFF, text = "Edit: nil", size = 16}
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

  delete_ui_node("box_edit")

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
  if key == string.byte('C') and action == 1 then
    local insertion_voxel = self:get_ray_insertion_voxel()
    if not box_edit_start and insertion_voxel then
      box_edit_start = insertion_voxel
    elseif insertion_voxel then
      local style = self.palette_styles[self.palette_selection]
      self:insert_voxel_box(box_edit_start, insertion_voxel, style, false)
      box_edit_start = nil
    end
    self:update_ui()
  end
  if key == string.byte('X') and action == 1 then
    if box_edit_start then
      local insertion_voxel = self:get_ray_insertion_voxel()
      if insertion_voxel then
        local style = self.palette_styles[self.palette_selection]
        self:insert_voxel_box(box_edit_start, insertion_voxel, style, true)
        box_edit_start = nil
      end
    end
    self:update_ui()
  end
  if key == KEYS.escape and action == 1 then
    box_edit_start = nil
    self:update_ui()
  end
end

function module:on_update(dt)
  -- Apply delayed voxel edits.
  edit_delay_s = edit_delay_s + dt
  if is_mouse_pressed(0) then
    self:insert_voxel()
  elseif is_mouse_pressed(1) then
    self:remove_voxel()
  end
end

return module
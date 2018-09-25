var module = {}

var KEY_MAP = {
  'c': 67,
  'e': 69,
  'g': 71,
  'q': 81,
  'w': 87,
  's': 83,
  'a': 65,
  'd': 68,
  'space': 32,
  'pg_up': 266,
  'pg_dn': 267,
  'home': 268,
  'left_shift': 340,
};

var orientation = [0, 0];
var orientation_toggle = true;
var gravity_mode = false;
var collision_mode = false;
var jump_velocity = 0;

function any_key_pressed(keys) {
  for (var i = 0; i < keys.length; i += 1) {
    if (is_key_pressed(KEY_MAP[keys[i]])) {
      return true;
    }
  }
  return false;
}

function clamp(val, min, max) {
  return Math.max(min, Math.min(max, val));
}

function clampCameraMove(camera_pos, camera_move) {
  var x = camera_pos[0];
  var y = camera_pos[1];
  var z = camera_pos[2];
  var fx = x - parseInt(x);
  var fy = y - parseInt(y);
  var fz = z - parseInt(z);
  var lv = get_voxel(x - 1, y, z) != 0 ? 1 : 0;
  var rv = get_voxel(x + 1, y, z) != 0 ? 1 : 0;
  var dv = get_voxel(x, y - 2, z) != 0 ? 1 : 0;
  var uv = get_voxel(x, y + 1, z) != 0 ? 1 : 0;
  var fv = get_voxel(x, y, z - 1) != 0 ? 1 : 0;
  var bv = get_voxel(x, y, z + 1) != 0 ? 1 : 0;
  camera_move[0] = clamp(
    camera_move[0], lv * (1 - fx) - 1, 1 - rv * fx,
  );
  camera_move[1] = clamp(
    camera_move[1], dv * (1 - fy) - 1, 1 - uv * fy
  );
  camera_move[2] = clamp(
    camera_move[2], bv * (1 - fz) - 1, 1 - fv * fz,
  );
}

module.on_init = function () {
  set_cursor_visible(false);
}

module.on_key = function (key, scancode, action, mods) {
  if (key == KEY_MAP['e'] && action == 1) {
    orientation_toggle = !orientation_toggle;
    set_cursor_visible(!orientation_toggle);
  } else if (key == KEY_MAP['c'] && action == 1) {
    collision_mode = !collision_mode;
  } else if (key == KEY_MAP['g'] && action == 1) {
    gravity_mode = !gravity_mode;
  } else if (key == KEY_MAP['space'] && action == 1) {
    jump_velocity = 100.0;
  }
}

module.on_click = function (button, action, mods) {
  if (button == 0 && action == 1) {
    var camera_pos = get_camera_pos();
    set_voxel(
      camera_pos[0],
      camera_pos[1],
      camera_pos[2],
      0xFFFFFFFF,
    );
  }
}

module.on_update = function (dt) {
  // Update the camera position based on input.
  if (gravity_mode || any_key_pressed(['w', 's', 'a', 'd', 'pg_up', 'pg_dn'])) {
    var camera_speed = is_key_pressed(KEY_MAP['left_shift']) ? 100 : 10;
    var camera_pos = get_camera_pos();
    var camera_view = get_camera_view();
    var camera_move = [0, 0, 0];

    // Calculate how much to move the camera based on key input.
    if (is_key_pressed(KEY_MAP['w'])) {
      camera_move[0] += camera_speed * dt * camera_view[0];
      camera_move[1] += camera_speed * dt * camera_view[1];
      camera_move[2] += camera_speed * dt * camera_view[2];
    }
    if (is_key_pressed(KEY_MAP['s'])) {
      camera_move[0] -= camera_speed * dt * camera_view[0];
      camera_move[1] -= camera_speed * dt * camera_view[1];
      camera_move[2] -= camera_speed * dt * camera_view[2];
    }
    if (is_key_pressed(KEY_MAP['a'])) {
      camera_move[0] += camera_speed * dt * camera_view[2];
      camera_move[2] -= camera_speed * dt * camera_view[0];
    }
    if (is_key_pressed(KEY_MAP['d'])) {
      camera_move[0] -= camera_speed * dt * camera_view[2];
      camera_move[2] += camera_speed * dt * camera_view[0];
    }
    if (is_key_pressed(KEY_MAP['pg_up'])) {
      camera_move[1] += camera_speed * dt;
    }
    if (is_key_pressed(KEY_MAP['pg_dn'])) {
      camera_move[1] -= camera_speed * dt;
    }

    // Calculate how much to move the camera based on gravity.
    if (gravity_mode) {
      var gravity_force = -0.5, terminal_velocity = -20.0;
      jump_velocity += gravity_force;
      jump_velocity = Math.max(jump_velocity, terminal_velocity);
      camera_move[1] += dt * jump_velocity;
    }

    // Calculate how much to move the camera based on collision.
    if (collision_mode || gravity_mode) {
      clampCameraMove(camera_move);
    }

    set_camera_pos(
      camera_pos[0] + camera_move[0],
      camera_pos[1] + camera_move[1],
      camera_pos[2] + camera_move[2],
    );
  }

  if (is_key_pressed(KEY_MAP['home'])) {
    set_camera_view(1, 0, 0);
  }

  // Update the camera view direction if the left mouse button is pressed.
  if (orientation_toggle) {
    var speed = 0.1 * dt;
    var cursor_pos = get_cursor_pos();
    var window_size = get_window_size();
    orientation[0] += speed * parseInt(0.5 * window_size[0] - cursor_pos[0]);
    orientation[1] += speed * parseInt(0.5 * window_size[1] - cursor_pos[1]);
    orientation[1] = clamp(orientation[1], -0.4 * Math.PI, 0.4 * Math.PI);

    // Reset the camera view based on the orientation angles.
    set_camera_angles(orientation[0], orientation[1]);

    // Set the cursor back to the center of the window.
    set_cursor_pos(0.5 * window_size[0], 0.5 * window_size[1]);
  }
}

module;
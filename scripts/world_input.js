var module = {};

var KEY_MAP = {
  'w': 87,
  's': 83,
  'a': 65,
  'd': 68,
  'pg_up': 266,
  'pg_dn': 267,
  'home': 268,
  'left_shift': 340,
};

var orientation = [0, 0];

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

module.on_update = function (dt) {
  // Update the camera position based on keyboard input.
  if (any_key_pressed(['w', 's', 'a', 'd', 'pg_up', 'pg_dn', 'home'])) {
    var speed = is_key_pressed(KEY_MAP['left_shift']) ? 100 : 10;
    var camera_pos = get_camera_pos();
    var camera_view = get_camera_view();
    if (is_key_pressed(KEY_MAP['w'])) {
      camera_pos[0] += speed * dt * camera_view[0];
      camera_pos[1] += speed * dt * camera_view[1];
      camera_pos[2] += speed * dt * camera_view[2];
    }
    if (is_key_pressed(KEY_MAP['s'])) {
      camera_pos[0] -= speed * dt * camera_view[0];
      camera_pos[1] -= speed * dt * camera_view[1];
      camera_pos[2] -= speed * dt * camera_view[2];
    }
    if (is_key_pressed(KEY_MAP['a'])) {
      camera_pos[0] += speed * dt * camera_view[2];
      camera_pos[2] -= speed * dt * camera_view[0];
    }
    if (is_key_pressed(KEY_MAP['d'])) {
      camera_pos[0] -= speed * dt * camera_view[2];
      camera_pos[2] += speed * dt * camera_view[0];
    }
    if (is_key_pressed(KEY_MAP['pg_up'])) {
      camera_pos[1] += speed * dt;
    }
    if (is_key_pressed(KEY_MAP['pg_dn'])) {
      camera_pos[1] -= speed * dt;
    }
    if (is_key_pressed(KEY_MAP['home'])) {
      camera_view = [1, 0, 0];
    }
    set_camera_pos(camera_pos[0], camera_pos[1], camera_pos[2]);
    set_camera_view(camera_view[0], camera_view[1], camera_view[2]);
  }

  // Update the camera view direction if the left mouse button is pressed.
  if (is_mouse_pressed(0)) {
    var speed = 0.1;
    var cursor_pos = get_cursor_pos();
    var window_size = get_window_size();
    orientation[0] += dt * speed * (0.5 * window_size[0] - cursor_pos[0]);
    orientation[1] += dt * speed * (0.5 * window_size[1] - cursor_pos[1]);
    orientation[1] = clamp(orientation[1], -0.4 * Math.PI, 0.4 * Math.PI);

    // Reset the camera view based on the orientation angles.
    set_camera_angles(orientation[0], orientation[1]);

    // Set the cursor back to the center of the window.
    set_cursor_pos(0.5 * window_size[0], 0.5 * window_size[1]);
  }
}

module;
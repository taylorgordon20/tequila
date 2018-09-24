var module = {};

var KEY_MAP = {
  'w': 87,
  's': 83,
  'a': 65,
  'd': 68,
  'pg_up': 266,
  'pg_dn': 267,
};

function any_key_pressed(keys) {
  for (var i = 0; i < keys.length; i += 1) {
    if (is_key_pressed(KEY_MAP[keys[i]])) {
      return true;
    }
  }
  return false;
}

module.on_update = function (dt) {
  // Update the camera position based on keyboard input.
  if (any_key_pressed(['w', 's', 'a', 'd', 'pg_up', 'pg_dn'])) {
    var speed = 10.0;
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
    set_camera_pos(camera_pos[0], camera_pos[1], camera_pos[2]);
  }
}

module;
KEYS = {
  space = 32,
  escape = 256,
  enter = 257,
  tab = 258,
  backspace = 259,
  down = 264,
  up = 265,
  pg_up = 266,
  pg_dn = 267,
  home = 268,
  left_shift = 340,
}

PI = 2 * math.asin(1)

function switch(condition, true_result, false_result)
  if condition then
    return true_result
  else 
    return false_result
  end
end

function clamp(val, min, max)
  return switch(val < min, min, switch(val > max, max, val))
end

function clamp_abs(val, size)
  return clamp(val, -math.abs(size), math.abs(size))
end

function truncate(val, threshold)
  return switch(math.abs(val) <= threshold, 0, val)
end
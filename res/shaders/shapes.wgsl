struct VertexInput {
  @location(0) position: vec2f,
  @location(1) coords: vec2f,
  @location(2) shapeType: u32,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) coords: vec2f,
  @location(1) @interpolate(flat) shapeType: u32,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    in.coords,
    in.shapeType
  );

  return out;
}

const pi = radians(180.0);

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  if (in.shapeType == 0) { // underline

  } else if (in.shapeType == 1) { // undercurl
    // x bounds = [0, 1]
    // y bounds = [1, 0]

    let girth = 0.20; // [0, 0.5]
    let fade = 0.5; // [0, 1]
    let dist = abs(
      in.coords.y -
      (cos(2 * pi * in.coords.x) * (0.5 - girth) + 0.5)
    );
    let value = clamp(girth - dist, 0.0, girth);
    let alpha = smoothstep(0, girth * fade, value);
    return vec4f(1, 1, 1, alpha);

  } else if (in.shapeType == 2) { // underdouble
    if (in.coords.y >= 1.0/3.0 && in.coords.y < 2.0/3.0) {
      return vec4f(1, 1, 1, 0);
    }

  } else if (in.shapeType == 3) { // underdotted
    let x = in.coords.x * 4.0;
    if (fract(x) >= 0.5) {
      return vec4f(1, 1, 1, 0);
    }

  } else if (in.shapeType == 4) { // underdashed
    if (in.coords.x >= 1.0/3.0 && in.coords.x < 2.0/3.0) {
      return vec4f(1, 1, 1, 0);
    }
  }

  return vec4f(1, 1, 1, 1);
}

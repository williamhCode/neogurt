struct VertexInput {
  @location(0) position: vec2f,
  @location(1) coords: vec2f,
  @location(2) color: vec4f,
  @location(3) lineType: u32,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) coords: vec2f,
  @location(1) color: vec4f,
  @location(2) @interpolate(flat) lineType: u32,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    in.coords,
    ToLinear(in.color),
    in.lineType
  );

  return out;
}

const pi = radians(180.0);

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  if (in.lineType == 0) { // underline

  } else if (in.lineType == 1) { // undercurl
    // x bounds = [0, 1]
    // y bounds = [1, 0]

    let girth = 0.2; // [0, 0.5]
    let fade = 0.5; // [0, 1]
    let dist = abs(
      in.coords.y -
      (cos(2 * pi * in.coords.x) * (0.5 - girth) + 0.5)
    );
    let value = clamp(girth - dist, 0.0, girth);
    let alpha = smoothstep(0, girth * fade, value);

    return vec4f(in.color.rgb, alpha);

  } else if (in.lineType == 2) { // underdouble
    if (in.coords.y >= 1.0/3.0 && in.coords.y < 2.0/3.0) {
      discard;
    }

  } else if (in.lineType == 3) { // underdotted
    let x = in.coords.x * 4.0;
    if (fract(x) >= 0.5) {
      discard;
    }

  } else if (in.lineType == 4) { // underdashed
    if (in.coords.x >= 1.0/3.0 && in.coords.x < 2.0/3.0) {
      discard;
    }
  }

  return in.color;
}

fn ToLinear(color: vec4f) -> vec4f {
  return vec4f(
    pow(color.r, 1.8f),
    pow(color.g, 1.8f),
    pow(color.b, 1.8f),
    color.a
  );
}

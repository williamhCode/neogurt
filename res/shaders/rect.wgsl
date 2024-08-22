struct VertexInput {
  @location(0) position: vec2f,
  @location(1) color: vec4f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) color: vec4f,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    ToLinear(in.color)
  );

  return out;
}

@fragment
fn fs_main(@location(0) color: vec4f) -> @location(0) vec4f {
  return color;
}

fn ToLinear(color: vec4f) -> vec4f {
  return vec4f(
    pow(color.r, 1.8f),
    pow(color.g, 1.8f),
    pow(color.b, 1.8f),
    color.a
  );
}

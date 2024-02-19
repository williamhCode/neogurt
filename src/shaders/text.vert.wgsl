struct VertexInput {
  @location(0) position: vec2f,
  @location(1) uv: vec2f,
  @location(2) foreground: vec4f,
  @location(3) background: vec4f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) uv: vec2f,
  @location(1) foreground: vec4f,
  @location(2) background: vec4f,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    in.uv, in.foreground, in.background,
  );

  return out;
}

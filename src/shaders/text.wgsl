struct VertexInput {
  @location(0) position: vec2f,
  @location(1) uv: vec2f,
  @location(2) foreground: vec4f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) uv: vec2f,
  @location(1) foreground: vec4f,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    in.uv, in.foreground,
  );

  return out;
}

@group(1) @binding(0) var fontTexture : texture_2d<f32>;
@group(1) @binding(1) var fontSampler : sampler;

struct FragmentInput {
  @location(0) uv: vec2f,
  @location(1) foreground: vec4f,
}

struct FragmentOutput {
  @location(0) color: vec4f,
  @location(1) mask: f32,
}

@fragment
fn fs_main(in: FragmentInput) -> FragmentOutput {
  var out: FragmentOutput;

  var color = textureSample(fontTexture, fontSampler, in.uv);
  out.color = in.foreground * color;

  if (out.color.a > 0.0) {
    out.mask = out.color.a;
  }

  return out;
}

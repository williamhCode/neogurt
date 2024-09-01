struct VertexInput {
  @location(0) position: vec2f,
  @location(1) regionCoords: vec2f,
  @location(2) foreground: vec4f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) uv: vec2f,
  @location(1) foreground: vec4f,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;
@group(1) @binding(0) var<uniform> textureSize : vec2f; // size of texture atlas

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let uv = in.regionCoords / textureSize;
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    uv, ToLinear(in.foreground)
  );

  return out;
}

struct FragmentInput {
  @location(0) uv: vec2f,
  @location(1) foreground: vec4f,
}

struct FragmentOutput {
  @location(0) color: vec4f,
}

@group(2) @binding(0) var fontTexture : texture_2d<f32>;
@group(2) @binding(1) var fontSampler : sampler;

@fragment
fn fs_main(in: FragmentInput) -> FragmentOutput {
  var out: FragmentOutput;

  out.color = textureSample(fontTexture, fontSampler, in.uv);
  out.color = in.foreground * out.color;

  return out;
}

fn ToLinear(color: vec4f) -> vec4f {
  return vec4f(
    pow(color.r, 1.8f),
    pow(color.g, 1.8f),
    pow(color.b, 1.8f),
    color.a
  );
}

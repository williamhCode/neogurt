struct VertexInput {
  @location(0) position: vec2f,
  @location(1) regionCoords: vec2f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) uv: vec2f,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;
@group(1) @binding(0) var<uniform> textureAtlasSize : vec2f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  return VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    in.regionCoords / textureAtlasSize
  );
}

struct FragmentInput {
  @location(0) uv: vec2f,
}

struct FragmentOutput {
  @location(0) mask: f32,
}

@group(1) @binding(1) var fontTexture : texture_2d<f32>;
@group(1) @binding(2) var fontSampler : sampler;

@fragment
fn fs_main(in: FragmentInput) -> FragmentOutput {
  let alpha = textureSample(fontTexture, fontSampler, in.uv).a;
  return FragmentOutput(alpha);
}

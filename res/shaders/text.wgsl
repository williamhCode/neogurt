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
@group(1) @binding(0) var<uniform> gamma: f32;
@group(2) @binding(0) var<uniform> textureSize : vec2f; // size of texture atlas


@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let uv = in.regionCoords / textureSize;
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    uv,
    in.foreground
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

@group(3) @binding(0) var fontTexture : texture_2d<f32>;
@group(3) @binding(1) var fontSampler : sampler;

@fragment
fn fs_main(in: FragmentInput) -> FragmentOutput {
  var out: FragmentOutput;

  // R8Unorm texture
  let alpha = textureSample(fontTexture, fontSampler, in.uv).r;
  out.color = ToLinear(vec4f(in.foreground.rgb, in.foreground.a * alpha));

  return out;
}

fn ToLinear(color: vec4f) -> vec4f {
  return vec4f(
    pow(color.r, gamma),
    pow(color.g, gamma),
    pow(color.b, gamma),
    color.a
  );
}

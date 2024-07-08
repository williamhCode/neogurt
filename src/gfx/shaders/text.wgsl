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
@group(1) @binding(0) var<uniform> textureAtlasSize : vec2f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let uv = in.regionCoords / textureAtlasSize;
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
  // @location(1) mask: f32,
}

@group(1) @binding(1) var fontTexture : texture_2d<f32>;
@group(1) @binding(2) var fontSampler : sampler;

@fragment
fn fs_main(in: FragmentInput) -> FragmentOutput {
  var out: FragmentOutput;

  out.color = textureSample(fontTexture, fontSampler, in.uv);
  out.color = in.foreground * out.color;

  // let alpha = textureSample(fontTexture, fontSampler, in.uv).a;
  // out.color = vec4f(in.foreground.rgb, alpha);

  // if (out.color.a > 0.0) {
  //   out.mask = out.color.a;
  // }

  return out;
}

fn ToLinear(color: vec4f) -> vec4f {
  return vec4f(
    pow(color.r, 2.2f),
    pow(color.g, 2.2f),
    pow(color.b, 2.2f),
    color.a
  );
}

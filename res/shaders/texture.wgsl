struct VertexInput {
  @location(0) position: vec2f,
  @location(1) uv: vec2f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) uv: vec2f,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    in.uv,
  );

  return out;
}

@group(1) @binding(0) var texture : texture_2d<f32>;
@group(1) @binding(1) var textureSampler : sampler;

@fragment
fn fs_main(@location(0) uv: vec2f) -> @location(0) vec4f {
  var color = textureSample(texture, textureSampler, uv);
  // if (color.a == 0.0f) {
  //   discard;
  // }
  return AdjustAlpha(color);
}

// adjust alpha due to srgb blending
fn AdjustAlpha(color: vec4f) -> vec4f {
  // return vec4f(color.rgb, pow(color.a, 1.0f / 1.8f));
  return color;
}

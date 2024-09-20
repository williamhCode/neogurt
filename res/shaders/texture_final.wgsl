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

@group(1) @binding(0) var<uniform> gamma: f32;

@group(2) @binding(0) var texture : texture_2d<f32>;
@group(2) @binding(1) var textureSampler : sampler;

@fragment
fn fs_main(@location(0) uv: vec2f) -> @location(0) vec4f {
  var color = textureSample(texture, textureSampler, uv);

  color = Premult(color);
  color = ToSrgb(color);
  return color;
}

fn ToSrgb(color: vec4f) -> vec4f {
  return vec4f(
    pow(color.r, 1.0f / gamma),
    pow(color.g, 1.0f / gamma),
    pow(color.b, 1.0f / gamma),
    color.a
  );
}

fn Premult(color: vec4f) -> vec4f {
  return vec4f(color.rgb * color.a, color.a);
}


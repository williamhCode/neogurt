struct VertexInput {
  @location(0) position: vec2f,
  @location(1) foreground: vec4f,
  @location(2) background: vec4f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) maskPos: vec4f,
  @location(1) foreground: vec4f,
  @location(2) background: vec4f,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;
@group(1) @binding(0) var<uniform> maskViewProj: mat4x4f;
@group(2) @binding(0) var<uniform> maskPos: vec2f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    maskViewProj * vec4f(in.position - maskPos, 0.0, 1.0),
    in.foreground,
    in.background,
  );

  return out;
}

@group(3) @binding(0) var maskTexture: texture_2d<f32>;
@group(3) @binding(1) var maskTextureSampler: sampler;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  var maskPos = in.maskPos; maskPos.y = -maskPos.y;
  let uv = (maskPos.xy / maskPos.w) * 0.5f + 0.5f;
  let mask = textureSample(maskTexture, maskTextureSampler, uv).r;

  var color = in.background;
  color.a = 1.0 - mask;
  color = Blend(color, in.foreground);

  return color;
}

fn Blend(src: vec4f, dst: vec4f) -> vec4f {
  let outAlpha = src.a + dst.a * (1.0 - src.a);
  let outColor = src.rgb * src.a + dst.rgb * (1.0 - src.a);

  return vec4f(outColor, outAlpha);
}

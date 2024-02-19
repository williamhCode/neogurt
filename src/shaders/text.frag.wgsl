struct VertexOutput {
  @location(0) uv: vec2f,
  @location(1) foreground: vec4f,
  @location(2) background: vec4f,
}

@group(1) @binding(0) var fontTexture : texture_2d<f32>;
@group(1) @binding(1) var fontSampler : sampler;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  let color = textureSample(fontTexture, fontSampler, in.uv);
  // let foreground = in.foreground * color;

  return color;
}

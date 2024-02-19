struct VertexOutput {
  @location(0) uv: vec2f,
  @location(1) foreground: vec4f,
}

@group(1) @binding(0) var fontTexture : texture_2d<f32>;
@group(1) @binding(1) var fontSampler : sampler;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  var color = textureSample(fontTexture, fontSampler, in.uv);
  color = in.foreground * color;

  return color;
}

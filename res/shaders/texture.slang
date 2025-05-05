import utils;

struct VertexIn {
  float2 position;
  float2 uv;
}

struct VertexOut {
  float4 position : SV_POSITION;
  float2 uv;
}

ParameterBlock<float4x4> viewProj;

[shader("vertex")]
VertexOut vs_main(VertexIn in) {
  VertexOut out;
  out.position = mul(viewProj, float4(in.position, 0.0, 1.0));
  out.uv = in.uv;

  return out;
}

#ifndef FINAL
ParameterBlock<float4> defaultBgLinear;
#endif
ParameterBlock<TextureInfo> texture;

[shader("fragment")]
float4 fs_main(float2 uv) {
  float4 color = texture.Sample(uv);

#ifndef FINAL
  color = select(color.a == 0.0, defaultBgLinear, color);
#else
  color = Premult(color);
  color = ToSrgb(color);
#endif

  return color;
}

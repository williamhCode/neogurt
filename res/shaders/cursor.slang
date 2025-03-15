import utils;

struct VertexIn {
  float2 position;
  float4 foreground;
  float4 background;
};

struct VertexOut {
  float4 position : SV_Position;
  float4 maskPos;
  float4 foreground;
  float4 background;
};

ParameterBlock<float4x4> viewProj;
ParameterBlock<float4x4> maskViewProj;
ParameterBlock<float2> maskPos;

[shader("vertex")]
VertexOut vs_main(VertexIn in) {
  VertexOut out;
  out.position = mul(viewProj, float4(in.position, 0.0, 1.0));
  out.maskPos = mul(maskViewProj, float4(in.position - maskPos, 0.0, 1.0));
  out.foreground = ToLinear(in.foreground);
  out.background = ToLinear(in.background);

  return out;
}

ParameterBlock<TextureInfo> maskTexture;

[shader("fragment")]
float4 fs_main(VertexOut in) {
  float2 maskPos = {in.maskPos.x, -in.maskPos.y};
  float2 uv = (maskPos / in.maskPos.w) * 0.5 + 0.5;
  float mask = maskTexture.Sample(uv).r;

#ifdef EMOJI
  float4 color = in.background;
  if (mask >= 0.8) {
    discard;
  }
#else
  float4 color = in.background;
  color.a = 1.0 - mask;
  color = Blend(color, in.foreground);
#endif
  return ToSrgb(color);
}

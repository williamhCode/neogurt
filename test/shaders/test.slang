import utils;

struct VertexIn {
  float2 position;
  float2 regionCoords;
  float4 foreground;
}

struct VertexOut {
  float4 position : SV_Position;
  float2 uv;
  float4 foreground;
}

ParameterBlock<float4x4> viewProj;
ParameterBlock<float> gamma;
ParameterBlock<float2> textureSize;

[shader("vertex")]
VertexOut vs_main(VertexIn in)
{
  VertexOut out;
  out.position = mul(viewProj, float4(in.position, 0.0, 1.0));
  out.uv = in.regionCoords / textureSize;
  out.foreground = in.foreground;

  return out;
}

ParameterBlock<TextureInfo> texture;

[shader("fragment")]
float4 fs_main(VertexOut in) {
#ifdef EMOJI
  let color = texture.Sample(in.uv);
#else
  let alpha = texture.Sample(in.uv).r;
  let color = float4(in.foreground.rgb, in.foreground.a * alpha);
#endif

  return ToLinear(color);
}

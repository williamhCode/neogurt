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
  let uv = in.regionCoords / textureSize;
  let out = VertexOut(
    mul(viewProj, float4(in.position, 0.0, 1.0)),
    uv,
    in.foreground
  );

  return out;
}

struct FragmentIn {
  float2 uv;
  float4 foreground;
}

struct FragmentOut {
  float4 color;
}

struct TextureInfo {
  Texture2D texture;
  SamplerState sampler;
}

ParameterBlock<TextureInfo> txr;

[shader("fragment")]
FragmentOut fs_main(FragmentIn in) {
  FragmentOut out;

#ifdef EMOJI
  out.color = txr.texture.Sample(txr.sampler, in.uv);
#else
  let alpha = txr.texture.Sample(txr.sampler, in.uv).r;
  out.color = float4(in.foreground.rgb, in.foreground.a * alpha);
#endif

  out.color = ToLinear(out.color);
  return out;
}

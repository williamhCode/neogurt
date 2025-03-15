import utils;

struct VertexIn {
  float2 position;
  float2 regionCoords;
};

struct VertexOut {
  float4 position : SV_Position;
  float2 uv;
};

ParameterBlock<float4x4> viewProj;
ParameterBlock<float2> textureSize;

[shader("vertex")]
VertexOut vs_main(VertexIn in) {
  VertexOut out;
  out.position = mul(viewProj, float4(in.position, 0.0, 1.0));
  out.uv = in.regionCoords / textureSize;

  return out;
}

ParameterBlock<TextureInfo> texture;

[shader("fragment")]
float4 fs_main(float2 uv) {
#ifdef EMOJI
  return texture.Sample(uv).a;
#else
  return texture.Sample(uv).r;
#endif
}

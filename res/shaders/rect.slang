import utils;

struct VertexIn {
  float2 position;
  float4 color;
}

struct VertexOut {
  float4 position : SV_POSITION;
  float4 color;
}

ParameterBlock<float4x4> viewProj;

[shader("vertex")]
VertexOut vs_main(VertexIn in) {
  VertexOut out;
  out.position = mul(viewProj, float4(in.position, 0.0, 1.0));
  out.color = ToLinear(in.color);

  return out;
}

[shader("fragment")]
float4 fs_main(float4 color) {
  return color;
}

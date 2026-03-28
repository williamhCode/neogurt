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

ParameterBlock<TextureInfo> texture;
ParameterBlock<float> time;

[shader("fragment")]
float4 fs_main(VertexOut in) {
  // Barrel distortion
  float2 cuv = in.uv * 2.0 - 1.0;
  cuv += cuv * cuv.yx * cuv.yx * 0.07;
  float2 uv = cuv * 0.5 + 0.5;

  bool outOfBounds = uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0;

  // preEffectsTexture already contains display-ready (sRGB) values — sample and pass through
  float4 color = texture.Sample(clamp(uv, float2(0.0), float2(1.0)));

  // Animated film grain
  float2 noiseCoord = in.position.xy + float2(time * 7919.0, time * 6271.0);
  float grain = frac(sin(dot(noiseCoord, float2(12.9898, 78.233))) * 43758.5453);
  color.rgb += (grain - 0.5) * 0.12;

  // Scanlines
  float scanline = 0.85 + 0.15 * sin(in.position.y * 3.14159265);
  color.rgb *= scanline;

  // Vignette
  float2 vig = uv * (1.0 - uv.yx);
  color.rgb *= pow(vig.x * vig.y * 16.0, 0.25);

  if (outOfBounds) return float4(0.0, 0.0, 0.0, 0.0);
  return color;
}

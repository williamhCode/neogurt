import utils;

struct VertexIn {
  float2 position;
  float2 size;
  float2 coords;
  float4 color;
  uint shapeType;
}

struct VertexOut {
  float4 position : SV_POSITION;
  float2 size;
  float2 coords;
  float4 color;
  nointerpolation uint shapeType;
}

ParameterBlock<float4x4> viewProj;

[shader("vertex")]
VertexOut vs_main(VertexIn in) {
  VertexOut out;
  out.position = mul(viewProj, float4(in.position, 0.0, 1.0));
  out.size = in.size;
  out.coords = in.coords;
  out.color = ToLinear(in.color);
  out.shapeType = in.shapeType;

  return out;
}

static const float pi = radians(180.0);

[shader("fragment")]
float4 fs_main(VertexOut in) {
  float x = in.coords.x;
  float y = in.coords.y;
  float w = in.size.x;
  float h = in.size.y;
  float xn = x / w;
  float yn = y / h;

  float alpha = 1;

  switch (in.shapeType) {
    case 0: { // underline
      break;
    }
    case 1: { // undercurl
      float girth = h * 0.2; // distance from top to peak of cosine wave
      float fade = min(girth * 0.5, 0.5);

      float ycenter = cos(2 * pi * xn) * (h/2 - girth) + h/2;
      float dist = abs(y - ycenter)
        // make thickness more uniform
        - pow(sin(2 * pi * xn), 2) * h * 0.04;

      alpha = 1 - smoothstep(girth - fade, girth, dist);
      break;
    }
    case 2: { // underdouble
      if (1/3. <= yn && yn < 2/3.) {
        alpha = 0;
      }
      break;
    }
    case 3: { // underdotted
      if (fract(xn * 4) >= 0.5) {
        alpha = 0;
      }
      break;
    }
    case 4: { // underdashed
      if (1/3. <= xn && xn < 2/3.) {
        alpha = 0;
      }
      break;
    }
  }

  return float4(in.color.rgb, alpha);
}

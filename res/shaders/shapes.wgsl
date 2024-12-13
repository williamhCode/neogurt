struct VertexInput {
  @location(0) position: vec2f,
  @location(1) size: vec2f,
  @location(2) coords: vec2f,
  @location(3) color: vec4f,
  @location(4) shapeType: u32,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) size: vec2f,
  @location(1) coords: vec2f,
  @location(2) color: vec4f,
  @location(3) @interpolate(flat) shapeType: u32,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;
@group(1) @binding(0) var<uniform> gamma: f32;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    in.size,
    in.coords,
    ToLinear(in.color),
    in.shapeType
  );

  return out;
}

fn ToLinear(color: vec4f) -> vec4f {
  return vec4f(
    pow(color.r, gamma),
    pow(color.g, gamma),
    pow(color.b, gamma),
    color.a
  );
}

const pi = radians(180.0);

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  let x = in.coords.x;
  let y = in.coords.y;
  let w = in.size.x; 
  let h = in.size.y;
  let xn = x / w;
  let yn = y / h;

  var alpha: f32 = 1;

  switch in.shapeType {
    default {}
    case 0 { // underline

    }
    case 1 { // undercurl
      let girth = h * 0.2; // distance from top to peak of cosine wave
      let fade = min(girth * 0.5, 0.5);

      let ycenter = cos(2 * pi * xn) * (h/2 - girth) + h/2;
      let dist = abs(y - ycenter)
        // make thickness more uniform
        - pow(sin(2 * pi * xn), 2) * h * 0.04;

      alpha = 1 - smoothstep(girth - fade, girth, dist);
    }
    case 2 { // underdouble
      if (1/3. <= yn && yn < 2/3.) {
        alpha = 0;
      }
    }
    case 3 { // underdotted
      if (fract(xn * 4) >= 0.5) {
        alpha = 0;
      }
    }
    case 4 { // underdashed
      if (1/3. <= xn && xn < 2/3.) {
        alpha = 0;
      }
    }
    case 5 { // braille circle
      let radius = w * 0.5;
      let fade = min(radius * 0.1, 1);

      let dist = distance(in.coords, in.size / 2.);
      alpha = 1 - smoothstep(radius - fade, radius, dist);
    }
  }

  return vec4f(in.color.rgb, alpha);
}

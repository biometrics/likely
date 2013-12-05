-- Likely Standard Library

-- Type abbreviations
null = likely.null
depth = likely.depth
signed = likely.signed
floating = likely.floating
u8 = likely.u8
u16 = likely.u16
u32 = likely.u32
u64 = likely.u64
i8 = likely.i8
i16 = likely.i16
i32 = likely.i32
i64 = likely.i64
f16 = likely.f16
f32 = likely.f32
f64 = likely.f64
parallel = likely.parallel
multi_channel = likely.multi_channel
multi_column = likely.multi_column
multi_row = likely.multi_row
multi_frame = likely.multi_frame
saturation = likely.saturation
reserved = likely.reserved

i = "i"
c = "c"
x = "x"
y = "y"
t = "t"

-- Core functions
function chain(x,y)
  return function(w) return tostring(y{tostring(x{w})}) end
end

closure = likely.closure

new = closure(
  nil,
  "Create an empty matrix",
  {{"type", "matrix type", f32},
   {"channels", "matrix channels", 1},
   {"columns", "matrix columns", 1},
   {"rows", "matrix rows", 1},
   {"frames", "matrix frames", 1},
   {"data", "address of existing data buffer", 0},
   {"copy", "copy the data buffer", false}},
  likely.new)

scalar = closure(
  nil,
  "Create a matrix from a number",
  {{"value", "matrix value"}},
  likely.scalar)

read = closure(
  nil,
  "Create a matrix from a file",
  {{"file_name", "path to file on disk"}},
  likely.read)

compile = closure(
  nil,
  "Create kernel",
  {{"function", "function expression"}},
  likely.compile)

-- Examples
likely.examples =
{
[[
-- Hello World
-- Source code is re-executed as you type\dream{activate:message}
message = "Hello World!"

-- \dream{CTRL}+click variables to display values
lenna = read("data/misc/lenna.tiff")

-- \dream{CTRL}+scroll to edit numerical constants
dark_lenna = divide{2}(lenna)
]],
[[
-- Function Calls
-- Arguments can be specified by name...\dream{activate:new}
m = new{rows=512, columns=512, type=f32}()

-- ...or anonymously in their canonical order.
n = new(f32, 1, 512, 512)

-- Arguments not specified by name are assigned to the first parameter without a bound value. If all parameters are bound then arguments will override the left-most values.
]],
[[
-- Closures
m = new{rows=512, columns=512, type=i8}()

-- The braces {} syntax returns a new function with the bound arguments.\dream{activate:set}\dream{activate:setZero}
setZero = set{0}

-- The function is just-in-time compiled using the bound arguments before execution.\dream{activate:n}
n = setZero(m)

-- set{0}(m)
--     ^  ^
--     |  Run-time arguments
--     Compile-time arguments
]],
[[
-- Arithmetic
lenna = read("data/misc/lenna.tiff")

-- Saturation arithmetic is part of the type system
lenna.saturation = true

-- Likely will automatically apply operations to all elements in a matrix
added = add(32,lenna)
subtracted = subtract(32,lenna)
multiplied = multiply(2,lenna)
divided = divide(2,lenna)
]],
[[
-- Currying
m = read("data/misc/lenna.tiff")
m.saturation = true

-- Likely supports currying to compose higher order functions\dream{activate:n}
fma = multiply{2} .. add{3}

n = fma(m)
]]
}

-- Utility functions
function likely.s(...)
  return "(" .. table.concat({...}, " ") .. ")"
end

-- Basic arithmetic
set = closure(
  function(value, matrix)
    return value
  end,
  "Assignment",
  {{"value", "numerical constant"},
   {"matrix", "used to determine output size and type"}})

add = closure(
  function(rhs, lhs) return likely.s("+", lhs, rhs) end,
  "Arithmetic addition",
  {{"rhs", "right hand side"},
   {"lhs", "left hand side"}})

subtract = closure(
  function(rhs, lhs) return likely.s("-", lhs, rhs) end,
  "Arithmetic subtraction",
  {{"rhs", "right hand side"},
   {"lhs", "left hand side"}})

multiply = closure(
  function(rhs, lhs) return likely.s("*", lhs, rhs) end,
  "Arithmetic multiplication",
  {{"rhs", "right hand side"},
   {"lhs", "left hand side"}})

divide = closure(
  function(rhs, lhs)
    if rhs == 0 then
      error("Denominator is zero!")
    else
      return likely.s("/", lhs, rhs)
    end
  end,
  "Arithmetic division",
  {{"rhs", "left hand side (denominator)"},
   {"lhs", "right hand side (numerator)"}})

-- Basic math
sqrt = closure(
  function(x) return likely.s("sqrt", x) end,
  "Square root: f(x) = x ** 0.5",
  {{"x", "operand"}})

powi = closure(
  function(n, x) return likely.s("powi", x, n) end,
  "Integer power: f(x) = x ** n",
  {{"x", "base"},
   {"n", "integer exponent"}})

sin = closure(
  function(x) return likely.s("sin", x) end,
  "Sine: f(x) = sin(x)",
  {{"x", "radians"}})

cos = closure(
  function(x) return likely.s("cos", x) end,
  "Cosine: f(x) = cos(x)",
  {{"x", "operand"}})

pow = closure(
  function(n, x) return likely.s("pow", x, n) end,
  "Power: f(x) = x ** n",
  {{"x", "base"},
   {"n", "exponent"}})

exp = closure(
  function(x) return likely.s("exp", x) end,
  "Base-e exponential: f(x) = e ** x",
  {{"x", "exponent"}})

exp2 = closure(
  function(x) return likely.s("exp2", x) end,
  "Base-2 exponential: f(x) = 2 ** x",
  {{"x", "exponent"}})

log = closure(
  function(x) return likely.s("log", x) end,
  "Natural logarithm: f(x) = log_e(x)",
  {{"x", "operand"}})

log10 = closure(
  function(x) return likely.s("log10", x) end,
  "Base-10 logarithm: f(x) = log_10(x)",
  {{"x", "operand"}})

log2 = closure(
  function(x) return likely.s("log2", x) end,
  "Base-2 logarithm: f(x) = log_2(x)",
  {{"x", "operand"}})

fma = closure(
  function(a, c, x) return likely.s("fma", a, x, c) end,
  "Fused multiply-add: f(a, b, x) = a * x + c",
  {{"a", "operand"},
   {"c", "operand"},
   {"x", "operand"}})

fabs = closure(
  function(x) return likely.s("fabs", x) end,
  "Floating absolute value: f(x) = |x|",
  {{"x", "operand"}})

copysign = closure(
  function(s, m) return likely.s("copysign", m, s) end,
  "Floating copy sign: f(x) = copysign(m, s)",
  {{"s", "value containing sign"},
   {"m", "value containing magnitude"}})

floor = closure(
  function(x) return likely.s("floor", x) end,
  "Floating floor: f(x) = floor(x)",
  {{"x", "operand"}})

ceil = closure(
  function(x) return likely.s("ceil", x) end,
  "Floating ceil: f(x) = ceil(x)",
  {{"x", "operand"}})

trunc = closure(
  function(x) return likely.s("trunc", x) end,
  "Floating round toward zero: f(x) = x > 0 ? floor(x) : ceil(x)",
  {{"x", "operand"}})

rint = closure(
  function(x) return likely.s("rint", x) end,
  "Floating round to nearest integer: f(x) = rint(x)",
  {{"x", "operand"}})

nearbyint = closure(
  function(x) return likely.s("nearbyint", x) end,
  "Floating round to nearest integer: f(x) = nearbyint(x)",
  {{"x", "operand"}})

round = closure(
  function(x) return likely.s("round", x) end,
  "Floating round to nearest integer: f(x) = round(x)",
  {{"x", "operand"}})

cast = closure(
  function(type, x) return likely.s("cast", x, type) end,
  "Cast matrix to another type",
  {{"type", "Likely type"},
   {"x", "operand"}})

lt = closure(
  function(rhs, lhs) return likely.s("<", lhs, rhs) end,
  "Check if lhs is less than rhs",
  {{"rhs", "right hand side"},
   {"lhs", "left hand side"}})

le = closure(
  function(rhs, lhs) return likely.s("<=", lhs, rhs) end,
  "Check if lhs is less than or equal to rhs",
  {{"rhs", "right hand side"},
   {"lhs", "left hand side"}})

gt = closure(
  function(rhs, lhs) return likely.s(">", lhs, rhs) end,
  "Check if lhs is greater than rhs",
  {{"rhs", "right hand side"},
   {"lhs", "left hand side"}})

ge = closure(
  function(rhs, lhs) return likely.s(">=", lhs, rhs) end,
  "Check if lhs is greater than or equal to rhs",
  {{"rhs", "right hand side"},
   {"lhs", "left hand side"}})

eq = closure(
  function(rhs, lhs) return likely.s("==", lhs, rhs) end,
  "Check if lhs is equal to rhs",
  {{"rhs", "right hand side"},
   {"lhs", "left hand side"}})

ne = closure(
  function(rhs, lhs) return likely.s("!=", lhs, rhs) end,
  "Check if lhs is not equal to rhs",
  {{"rhs", "right hand side"},
   {"lhs", "left hand side"}})

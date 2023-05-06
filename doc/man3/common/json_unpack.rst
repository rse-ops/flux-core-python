DECODING JSON PAYLOADS
======================

Flux API functions that are based on Jansson's json_unpack()
accept the following tokens in their format string.
The type in parenthesis denotes the resulting JSON type, and
the type in brackets (if any) denotes the C type that is expected as
the corresponding argument or arguments.

**s** (string)['const char \*']
   Convert a JSON string to a pointer to a null terminated UTF-8 string.
   The resulting string is extracted by using 'json_string_value()'
   internally, so it exists as long as there are still references to the
   corresponding JSON string.

**s%** (string)['const char \*', 'size_t']
   Convert a JSON string to a pointer to a null terminated UTF-8
   string and its length.

**n** (null)
   Expect a JSON null value. Nothing is extracted.

**b** (boolean)['int']
   Convert a JSON boolean value to a C int, so that *true* is converted to 1
   and *false* to 0.

**i** (integer)['int']
   Convert a JSON integer to a C int.

**I** (integer)['int64_t']
   Convert a JSON integer to a C int64_t.
   Note: Jansson expects a json_int_t here without committing to a size,
   but Flux guarantees that this is a 64-bit integer.

**f** (real)['double']
   Convert JSON real to a C double.

**F** (real)['double']
   Convert JSON number (integer or real) to a C double.

**o** (any value)['json_t \*']
   Store a JSON value, with no conversion, to a json_t pointer.

**O** (any value)['json_t \*']
   Like **o**, but the JSON value's reference count is incremented.

**[fmt]** (array)
   Convert each item in the JSON array according to the inner format
   string. **fmt** may contain objects and arrays, i.e. recursive value
   extraction is supported.

**{fmt}** (object)
   Convert each item in the JSON object according to the inner format
   string **fmt**. The first, third, etc. format specifier represent a
   key, and must by **s**. The corresponding argument to unpack functions
   is read as the object key. The second, fourth, etc. format specifier
   represent a value and is written to the address given as the corresponding
   argument. Note that every other argument is read from and every other
   is written to. **fmt** may contain objects and arrays as values, i.e.
   recursive value extraction is supported. Any **s** representing a key
   may be suffixed with **?** to make the key optional. If the key is not
   found, nothing is extracted.

**!**
   This special format specifier is used to enable the check that all
   object and array items are accessed, on a per-value basis. It must
   appear inside an array or object as the last format specifier before
   the closing bracket or brace.

Whitespace, **:** (colon) and **,** (comma) are ignored.

These descriptions came from the Jansson 2.10 manual.

See also: Jansson API: Parsing and Validating Values: http://jansson.readthedocs.io/en/2.10/apiref.html#parsing-and-validating-values

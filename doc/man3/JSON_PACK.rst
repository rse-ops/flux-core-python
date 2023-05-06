ENCODING JSON PAYLOADS
======================

Flux API functions that are based on Jansson's json_pack()
accept the following tokens in their format string.
The type in parenthesis denotes the resulting JSON type, and
the type in brackets (if any) denotes the C type that is expected as
the corresponding argument or arguments.

**s** (string)['const char \*']
   Convert a null terminated UTF-8 string to a JSON string.

**s?** (string)['const char \*']
   Like **s**, but if the argument is NULL, outputs a JSON null value.

**s#** (string)['const char \*', 'int']
   Convert a UTF-8 buffer of a given length to a JSON string.

**s%** (string)['const char \*', 'size_t']
   Like **s#** but the length argument is of type size_t.

**+** ['const char \*']
   Like **s**, but concatenate to the previous string.
   Only valid after a string.

**+#** ['const char \*', 'int']
   Like **s#**, but concatenate to the previous string.
   Only valid after a string.

**+%** ['const char \*', 'size_t']
   Like **+#**, but the length argument is of type size_t.

**n** (null)
   Output a JSON null value. No argument is consumed.

**b** (boolean)['int']
   Convert a C int to JSON boolean value. Zero is converted to
   *false* and non-zero to *true*.

**i** (integer)['int']
   Convert a C int to JSON integer.

**I** (integer)['int64_t']
   Convert a C int64_t to JSON integer.
   Note: Jansson expects a json_int_t here without committing to a size,
   but Flux guarantees that this is a 64-bit integer.

**f** (real)['double']
   Convert a C double to JSON real.

**o** (any value)['json_t \*']
   Output any given JSON value as-is. If the value is added to an array
   or object, the reference to the value passed to **o** is stolen by the
   container.

**O** (any value)['json_t \*']
   Like **o**, but the argument's reference count is incremented. This
   is useful if you pack into an array or object and want to keep the reference
   for the JSON value consumed by **O** to yourself.

**o?**, **O?** (any value)['json_t \*']
   Like **o** and **O**, respectively, but if the argument is NULL,
   output a JSON null value.

**[fmt]** (array)
   Build an array with contents from the inner format string. **fmt** may
   contain objects and arrays, i.e. recursive value building is supported.

**{fmt}** (object)
   Build an object with contents from the inner format string **fmt**.
   The first, third, etc. format specifier represent a key, and must be a
   string as object keys are always strings. The second, fourth, etc.
   format specifier represent a value. Any value may be an object or array,
   i.e. recursive value building is supported.

Whitespace, **:** (colon) and **,** (comma) are ignored.

These descriptions came from the Jansson 2.10 manual.

See also: Jansson API: Building Values: http://jansson.readthedocs.io/en/2.10/apiref.html#building-values

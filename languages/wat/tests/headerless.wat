;; A file whose fields sit at the top with no enclosing `(module ...)`: the
;; reference's own grammar accepts a document shaped this way too.
(type $id (func (param i32) (result i32)))
(func $id (export "id") (type $id)
  local.get 0)
(memory (export "mem") 1)

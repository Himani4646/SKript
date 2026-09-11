# Memory management

SKript values are ordinary C++ value objects. Heap-backed language objects—currently functions and lists—are stored in `std::shared_ptr` handles. Copying a SKript value copies that handle, incrementing the object's reference count; when the final handle leaves scope or is overwritten, RAII releases the object automatically.

This makes ownership deterministic and keeps the runtime portable without manual `new`/`delete` management. Environments also use shared ownership for captured scopes, allowing closures in the AST interpreter to retain their enclosing variables safely.

Reference counting does not reclaim cycles. In particular, a function can retain the environment that stores the same function for recursion. This small interpreter accepts that limitation for now and keeps the lifecycle design explicit. A production-ready SKript runtime should replace this with tracing garbage collection or redesign closure ownership around weak references and an environment-lifetime manager.

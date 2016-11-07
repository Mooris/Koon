%Integer = type { i32 }

declare void @exit(%Integer)
declare %Integer @start(i32, i8**)

define void @_start(i32 %ac, i8** %av) {
  %1 = alloca i32, align 4
  %2 = alloca i8**, align 8
  %3 = alloca %Integer, align 4

  store i32 %ac, i32* %1, align 4
  store i8** %av, i8*** %2, align 8
  %4 = load i32, i32* %1, align 4
  %5 = load i8**, i8*** %2, align 8
  %6 = call %Integer @start(i32 %4, i8** %5)
  call void @exit(%Integer %6)
  ret void
}



decl start -> Integer {
    var toto: Toto
    return |toto doStuff: 250|
}

class Toto {
    var lol = 5

    decl doStuff stuff: Integer -> Integer {
        return lol + stuff
    }
}

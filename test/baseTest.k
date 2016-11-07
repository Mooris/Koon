decl start -> Integer {
    var toto: Toto
    return |toto doStuff: 250|
}

class Toto {
    var lol = 93270

    decl doStuff stuff: Integer -> Integer {
    	|lol print|
        return lol + stuff
    }
}

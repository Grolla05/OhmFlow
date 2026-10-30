-- database.sql - schema do banco SQLite do OhmFlow (ver README.md desta pasta).
-- O Makefile embute este arquivo no executavel: mudar aqui muda o programa.

CREATE TABLE IF NOT EXISTS resistores (
    resistencia REAL    PRIMARY KEY NOT NULL CHECK (resistencia > 0),
    quantidade  INTEGER NOT NULL CHECK (quantidade BETWEEN 0 AND 2147483647)
);

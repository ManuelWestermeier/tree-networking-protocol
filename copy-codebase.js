const fs = require("fs");
const { join, extname } = require("path");

var out = "";

function readDir(path) {
    var lines = 0;

    fs.readdirSync(path).map((currentPath) => {
        var currentJoinedPath = join(path, currentPath);
        if (fs.statSync(currentJoinedPath).isFile()) {
            const en = extname(currentPath);
            if (en != ".ino" && en != ".cpp" && en != ".hpp") return;

            out += `\n//filepath: ${currentJoinedPath}\n${fs.readFileSync(
                currentJoinedPath,
                "utf-8"
            )}\n`;
        } else {
            readDir(currentJoinedPath);
        }
    });

    return lines;
}

readDir("./src/");
fs.writeFileSync("the-codebase-in-one-file.txt", out, "utf-8");
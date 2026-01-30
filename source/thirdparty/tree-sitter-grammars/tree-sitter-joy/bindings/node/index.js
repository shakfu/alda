const path = require("path");
const binding = require("node-gyp-build")(path.join(__dirname, "..", ".."));

module.exports = binding;

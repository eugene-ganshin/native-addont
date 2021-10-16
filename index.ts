interface ILtApi {
  openConnection: () => number;
  query: () => number;
  closeConnection: () => number;
}

const addon: ILtApi = require("bindings")("ltApi");

console.log(addon.openConnection());

// Need to pass env variables via JS?
// TS should have error handler and receive error data from c++ layer?
// // Can try to return error object from c++ layer and just throw it from node.

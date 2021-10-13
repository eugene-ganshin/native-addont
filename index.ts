type Greet = (num1: number, num2: number) => number;

const addon = require("bindings")("swap-nums");

export const greet: Greet = addon.swapNums;

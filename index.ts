interface IGreet {
  greetHello: (str: string) => void;
}

type GreetHello = (str: string) => void;

const addon: IGreet = require("bindings")("greet");

export const greet: GreetHello = addon.greetHello;

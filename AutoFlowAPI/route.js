module.exports = (app) => {
  const handler = require("./action");

  app.route("/").get(handler.main);
  app.route("/getAllInput").get(handler.getAllInput);

  app.route("/addInput").post(handler.addInput);

  app.route("/deleteInput/:id").delete(handler.deleteInput);
};

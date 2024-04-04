require("dotenv").config();
const express = require("express");
const cors = require("cors");

const app = express();
const bodyParser = require("body-parser");

const port = process.env.PORT;
const host = process.env.DB_HOST;

app.use(cors());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());

const route = require("./route");

route(app);

app.listen(port, () => {
  console.log(`API Server is running on http://${host}:${port}`);
});

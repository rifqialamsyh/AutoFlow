const mysql = require("mysql");
const { nanoid } = require("nanoid");
const db = require("./connection");

exports.main = (req, res) => {
  res.status(200).json({
    success: true,
    message: "AutoFlow API is running!",
  });
};

exports.addInput = (req, res) => {
  const id = nanoid(7);

  const now = new Date();
  const offsetMinutes = now.getTimezoneOffset();
  const offsetMilliseconds = offsetMinutes * 60 * 1000;

  const addInput = {
    id,
    temperature: req.body.temperature,
    soil_moisture: req.body.soil_moisture,
    amount_of_water: req.body.amount_of_water,
    date: new Date(now.getTime() - offsetMilliseconds)
      .toISOString()
      .slice(0, 19)
      .replace("T", " "),
  };

  db.query("INSERT INTO input SET ?", addInput, (err) => {
    if (err) {
      res.status(500).json({
        success: false,
        message: err.message,
      });
    } else {
      res.status(201).json({
        success: true,
        message: "Input data has been added successfully!",
        data: addInput,
      });
    }
  });
};

exports.getAllInput = (req, res) => {
  db.query("SELECT * FROM input", (err, result) => {
    if (err) {
      res.status(500).json({
        success: false,
        message: err.message,
      });
    } else {
      res.status(200).json({
        success: true,
        message: "Success",
        data: result,
      });
    }
  });
};

exports.deleteInput = (req, res) => {
  const id = req.params.id;

  db.query("DELETE FROM input WHERE id = ?", id, (err) => {
    if (err) {
      res.status(500).json({
        success: false,
        message: err.message,
      });
    } else {
      res.status(200).json({
        success: true,
        message: "Input data has been deleted successfully!",
      });
    }
  });
};

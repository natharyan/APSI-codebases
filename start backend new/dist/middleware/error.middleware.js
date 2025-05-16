"use strict";
/* eslint-disable no-unused-vars */
Object.defineProperty(exports, "__esModule", { value: true });
exports.errorHandler = void 0;
const errorHandler = (err, req, res, next) => {
    const status = err.statusCode || err.status || 500;
    res.status(status).send(err);
};
exports.errorHandler = errorHandler;

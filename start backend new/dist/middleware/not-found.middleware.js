"use strict";
/* eslint-disable no-unused-vars */
Object.defineProperty(exports, "__esModule", { value: true });
exports.notFoundHandler = void 0;
const notFoundHandler = (req, res, next) => {
    const message = 'Resource not found';
    res.status(404).send(message);
};
exports.notFoundHandler = notFoundHandler;

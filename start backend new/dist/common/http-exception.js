"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.default = void 0;
class HttpException extends Error {
    statusCode;
    status;
    message;
    error;
    constructor(statusCode, message, error) {
        super(message);
        this.statusCode = statusCode;
        this.message = message;
        this.error = error || null;
    }
}
exports.default = HttpException;

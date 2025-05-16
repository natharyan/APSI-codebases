"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const jsonwebtoken_1 = __importDefault(require("jsonwebtoken"));
const user_models_1 = require("../models/user.models");
// requireAuth middleware is applied to all routes except /login and /register
// to prevent unauthenticated users from accessing protected routes
const authenticateJWT = async (req, res, next) => {
    if (req.path === '/login' || req.path === '/register') {
        return next();
    }
    // verify user is authenticated
    const { authorization } = req.headers;
    if (!authorization) {
        return res.status(401).json({ error: 'Authorization token required' });
    }
    const token = authorization.split(' ')[1];
    try {
        // verify token
        const decodedToken = jsonwebtoken_1.default.verify(token, process.env.JWT_SECRET);
        const { _id } = decodedToken;
        req.user = await user_models_1.UserSchema.findOne({ _id }).select('_id');
        console.log('user is authenticated');
        next();
    }
    catch (error) {
        console.log('Error while authenticating : ', error);
        return res.status(401).json({ error: 'Request is not authorized' });
    }
};
exports.default = authenticateJWT;

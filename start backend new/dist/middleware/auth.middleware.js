"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.attachCookiesToResponse = exports.createRefreshToken = exports.createAccessToken = exports.authenticateJWT = void 0;
const jsonwebtoken_1 = __importDefault(require("jsonwebtoken"));
const user_models_1 = require("../models/user.models");
const token_models_1 = __importDefault(require("../models/token.models"));
const dotenv_1 = __importDefault(require("dotenv"));
dotenv_1.default.config();
const tokenSecret = process.env.JWT_SECRET || '';
const refreshTokenSecret = process.env.JWT_REFRESH_SECRET || '';
// authenticateJWT middleware is applied to all routes except /login and /register
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
        const decodedToken = jsonwebtoken_1.default.verify(token, tokenSecret);
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
exports.authenticateJWT = authenticateJWT;
const createAccessToken = (_id) => {
    return jsonwebtoken_1.default.sign({ _id }, tokenSecret, { expiresIn: '1d' });
};
exports.createAccessToken = createAccessToken;
const createRefreshToken = async (userId) => {
    // save useragent for specific device security 
    const refreshToken = jsonwebtoken_1.default.sign({ userId }, refreshTokenSecret, { expiresIn: '7d' });
    const token = new token_models_1.default({
        refreshToken,
        user: userId,
    });
    await token.save();
    return refreshToken;
};
exports.createRefreshToken = createRefreshToken;
const attachCookiesToResponse = ({ res, accessToken, refreshToken, }) => {
    const oneDay = 1000 * 60 * 60 * 24; // 1 day in milliseconds
    const longerExp = 1000 * 60 * 60 * 24 * 30; // 30 days in milliseconds
    res.cookie('accessToken', accessToken, {
        httpOnly: true,
        secure: process.env.NODE_ENV === 'production',
        signed: true,
        expires: new Date(Date.now() + oneDay),
    });
    res.cookie('refreshToken', refreshToken, {
        httpOnly: true,
        secure: process.env.NODE_ENV === 'production',
        signed: true,
        expires: new Date(Date.now() + longerExp),
    });
};
exports.attachCookiesToResponse = attachCookiesToResponse;

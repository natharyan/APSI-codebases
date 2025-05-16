import jwt, { JwtPayload, Secret } from 'jsonwebtoken'
import { Request, Response, NextFunction } from 'express'
import { UserSchema } from '../models/user.models'
import tokenModels from '../models/token.models'
import dotenv from 'dotenv'
dotenv.config()


// Extend the Request interface to include the user property
interface AuthenticatedRequest extends Request {
    user?: any
}

const tokenSecret = process.env.JWT_SECRET || ''
const refreshTokenSecret = process.env.JWT_REFRESH_SECRET || ''


// authenticateJWT middleware is applied to all routes except /login and /register
// to prevent unauthenticated users from accessing protected routes

const authenticateJWT = async (req: AuthenticatedRequest, res: Response, next: NextFunction) => {
    if (req.path === '/login' || req.path === '/register') {
        return next()
    }
    // verify user is authenticated
    const { authorization } = req.headers

    if (!authorization) {
        return res.status(401).json({ error: 'Authorization token required' })
    }

    const token = authorization.split(' ')[1]

    try {
        // verify token
        const decodedToken = jwt.verify(
            token,
            tokenSecret as Secret
        ) as JwtPayload
        const { _id } = decodedToken
        req.user = await UserSchema.findOne({ _id }).select('_id')
        console.log('user is authenticated')
        next()
    } catch (error) {
        console.log('Error while authenticating : ', error)
        return res.status(401).json({ error: 'Request is not authorized' })
    }
}

const createAccessToken = (_id: string) => {
    return jwt.sign({ _id }, tokenSecret, { expiresIn: '1d' })
}

const createRefreshToken = async (userId: any) => {
    // save useragent for specific device security 
    const refreshToken = jwt.sign({ userId }, refreshTokenSecret, { expiresIn: '7d' })

    const token = new tokenModels({
        refreshToken,

        user: userId,
    })

    await token.save()
    return refreshToken
}

const attachCookiesToResponse = ({
    res,
    accessToken,
    refreshToken,
}: {
    res: Response
    accessToken: string
    refreshToken: string
}) => {
    const oneDay = 1000 * 60 * 60 * 24 // 1 day in milliseconds
    const longerExp = 1000 * 60 * 60 * 24 * 30 // 30 days in milliseconds

    res.cookie('accessToken', accessToken, {
        httpOnly: true,
        secure: process.env.NODE_ENV === 'production',
        signed: true,
        expires: new Date(Date.now() + oneDay),
    })

    res.cookie('refreshToken', refreshToken, {
        httpOnly: true,
        secure: process.env.NODE_ENV === 'production',
        signed: true,
        expires: new Date(Date.now() + longerExp),
    })
}

export { authenticateJWT, createAccessToken, createRefreshToken, attachCookiesToResponse }